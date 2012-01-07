#include <gtkmm.h>
#include <lv2gui.hpp>
#include "visual_compressor.peg"
#include <gtkmm/drawingarea.h>
#include <cairomm/context.h>
#include <math.h>

using namespace sigc;
using namespace Gtk;

class CompVis : public Gtk::DrawingArea {
    public:
        CompVis();
        virtual ~CompVis();
        bool update_meter(uint32_t port, const void* buffer);

    protected:
        //Override default signal handler:
        virtual bool on_expose_event(GdkEventExpose* event);
        float rms;
        float threshold;
        float ratio;
        float gain;
        float rms_dB;
        float thresh_fraction;
        float rms_dB_fraction;
        float threshold_range;
};

CompVis::CompVis() {
    set_size_request(120, 150);
    threshold_range =  p_ports[p_threshold].max - p_ports[p_threshold].min;
}

CompVis::~CompVis() {
    printf ("visual compressor gui says bye bye\n");
}

bool CompVis::update_meter(uint32_t port, const void* buffer) {
    if (port == p_threshold) {
        threshold = *(float*)buffer;
    } else if (port == p_ratio) {
        ratio = *(float*)buffer;
    } else if (port == p_rms) {
        rms = *(float*)buffer;
    } else if (port == p_gain) {
        gain = *(float*)buffer;
    }
    Glib::RefPtr<Gdk::Window> win = get_window();
    if (win) {
        Gdk::Rectangle r(0, 0, get_allocation().get_width(),
        get_allocation().get_height());
        win->invalidate_rect(r, false);
    }
    return true;
}

bool CompVis::on_expose_event(GdkEventExpose* event) {
    Glib::RefPtr<Gdk::Window> window = get_window();
    Allocation allocation = get_allocation();
    const int width = allocation.get_width();
    const int height = allocation.get_height();
            
    Cairo::RefPtr<Cairo::Context> cr = window->create_cairo_context();
        
    // clip to the area indicated by the expose event so that we only redraw
    // the portion of the window that needs to be redrawn
    cr->rectangle(event->area.x, event->area.y,
    event->area.width, event->area.height);
    cr->clip();
           
    rms_dB = 20*log10(rms);
    thresh_fraction = (threshold -  p_ports[p_threshold].min)/threshold_range;
    rms_dB_fraction = (rms_dB - p_ports[p_threshold].min)/threshold_range;

    // Draw the graph
    cr->set_source_rgb(0.5,0.1,0.1);
    cr->set_line_width(2);
    cr->move_to(0,height);

    if (rms_dB <= threshold) {
        cr->line_to(rms_dB_fraction*width, height-rms_dB_fraction*height);
        cr->line_to(rms_dB_fraction*width, height);
        cr->line_to(0,height);
    } else {
        cr->line_to(thresh_fraction*width, height-thresh_fraction*height);
        cr->line_to(rms_dB_fraction*width, height-(thresh_fraction*height + height*(rms_dB_fraction-thresh_fraction)/ratio));
        cr->line_to(rms_dB_fraction*width, height);
        cr->line_to(0,height);
    }
    cr->fill();

    // draw the compression curve:
    cr->set_source_rgb(0.1,0.1,0.1);
    cr->move_to(0, height);
    cr->line_to(thresh_fraction*width, height-thresh_fraction*height);
    cr->line_to(width, height-(thresh_fraction*height + height*(1-thresh_fraction)/ratio));
    cr->stroke();

    // Draw the gain
    cr->set_source_rgb(0.1,0.8,0.1);
    cr->rectangle(0,(float)height - (float)height*gain, 10, height);
    cr->fill();
    return true;
}

class CompGUI : public LV2::GUI<CompGUI> {
public:
  
  CompGUI(const std::string& URI) {
    compvis = manage( new CompVis() );
    Table* table = manage(new Table(2, 7));
    table->attach(*manage(new Label("Threshold (dB) ", ALIGN_RIGHT)), 0, 1, 1, 2);
    table->attach(*manage(new Label("Ratio ", ALIGN_RIGHT)), 0, 1, 2, 3);
    table->attach(*manage(new Label("Input gain (dB) ", ALIGN_RIGHT)), 0, 1, 3, 4);
    table->attach(*manage(new Label("Makeup gain (dB) ", ALIGN_RIGHT)), 0, 1, 4, 5);
    table->attach(*manage(new Label("Attack time (s) ", ALIGN_RIGHT)), 0, 1, 5, 6);
    table->attach(*manage(new Label("Release time (s) ", ALIGN_RIGHT)), 0, 1, 6, 7);
    table->attach(*compvis, 0,2,0,1);
    threshold_scale = manage(new HScale(p_ports[p_threshold].min,
                p_ports[p_threshold].max, 0.1));
    ratio_scale = manage(new HScale(p_ports[p_ratio].min,
                p_ports[p_ratio].max, 0.1));
    input_gain_scale = manage(new HScale(p_ports[p_input_gain].min,
                p_ports[p_input_gain].max, 0.1));
    makeup_gain_scale = manage(new HScale(p_ports[p_makeup_gain].min,
                p_ports[p_makeup_gain].max, 0.1));
    attack_time_scale = manage(new HScale(p_ports[p_attack_time].min,
                p_ports[p_attack_time].max, 0.01));
    release_time_scale = manage(new HScale(p_ports[p_release_time].min,
                p_ports[p_release_time].max, 0.01));
    threshold_scale->set_size_request(100, -1);
    ratio_scale->set_size_request(100, -1);
    input_gain_scale->set_size_request(100, -1);
    makeup_gain_scale->set_size_request(100, -1);
    attack_time_scale->set_size_request(100, -1);
    release_time_scale->set_size_request(100, -1);
    slot<void> threshold_slot = compose(bind<0>(mem_fun(*this, &CompGUI::write_control), p_threshold),
                mem_fun(*threshold_scale, &HScale::get_value));
    slot<void> ratio_slot = compose(bind<0>(mem_fun(*this, &CompGUI::write_control), p_ratio),
                mem_fun(*ratio_scale, &HScale::get_value));
    slot<void> input_gain_slot = compose(bind<0>(mem_fun(*this, &CompGUI::write_control), p_input_gain),
                mem_fun(*input_gain_scale, &HScale::get_value));
    slot<void> makeup_gain_slot = compose(bind<0>(mem_fun(*this, &CompGUI::write_control), p_makeup_gain),
                mem_fun(*makeup_gain_scale, &HScale::get_value));
    slot<void> attack_time_slot = compose(bind<0>(mem_fun(*this, &CompGUI::write_control), p_attack_time),
                mem_fun(*attack_time_scale, &HScale::get_value));
    slot<void> release_time_slot = compose(bind<0>(mem_fun(*this, &CompGUI::write_control), p_release_time),
                mem_fun(*release_time_scale, &HScale::get_value));
    threshold_scale->signal_value_changed().connect(threshold_slot);
    ratio_scale->signal_value_changed().connect(ratio_slot);
    input_gain_scale->signal_value_changed().connect(input_gain_slot);
    makeup_gain_scale->signal_value_changed().connect(makeup_gain_slot);
    attack_time_scale->signal_value_changed().connect(attack_time_slot);
    release_time_scale->signal_value_changed().connect(release_time_slot);
    table->attach(*threshold_scale, 1, 2, 1, 2);
    table->attach(*ratio_scale, 1, 2, 2, 3);
    table->attach(*input_gain_scale, 1, 2, 3, 4);
    table->attach(*makeup_gain_scale, 1, 2, 4, 5);
    table->attach(*attack_time_scale, 1, 2, 5, 6);
    table->attach(*release_time_scale, 1, 2, 6, 7);
    add(*table);
  }
  
    void port_event(uint32_t port, uint32_t buffer_size, uint32_t format, const void* buffer) {
        if (port == p_threshold) {
            threshold_scale->set_value(*static_cast<const float*>(buffer));
        } else if (port == p_ratio) {
            ratio_scale->set_value(*static_cast<const float*>(buffer));
        } else if (port == p_input_gain) {
            input_gain_scale->set_value(*static_cast<const float*>(buffer));
        } else if (port == p_makeup_gain) {
            makeup_gain_scale->set_value(*static_cast<const float*>(buffer));
        } else if (port == p_attack_time) {
            attack_time_scale->set_value(*static_cast<const float*>(buffer));
        } else if (port == p_release_time) {
            release_time_scale->set_value(*static_cast<const float*>(buffer));
        }
        compvis->update_meter(port, buffer);
    }

protected:
    HScale* threshold_scale;
    HScale* ratio_scale;
    HScale* input_gain_scale;
    HScale* makeup_gain_scale;
    HScale* attack_time_scale;
    HScale* release_time_scale;
    CompVis* compvis;
};

static int _ = CompGUI::register_class("http://www.joebutton.co.uk/software/lv2/visual_compressor/gui");
