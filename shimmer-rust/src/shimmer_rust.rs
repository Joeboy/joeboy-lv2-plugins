extern crate libc;
use std::ptr;
use std::mem;

mod lv2;

pub enum PortIndex {
        Input= 0,
        Output = 1,
        Rate = 2,
        Depth = 3,
}

#[repr(C)]
struct ShimmerPlugin {
    input: *const f32,
    output: *mut f32,
    sample_rate: f64,
    shimmer_rate: *const f32,
    shimmer_depth: *const f32,
    shimmer_phase: f32,
    going_up: bool,
}


impl lv2::LV2Descriptor {

    pub extern fn instantiate(_descriptor: *const lv2::LV2Descriptor, sample_rate: f64, _bundle_path: *const u8, _features: *const lv2::LV2Feature)
                                -> lv2::Lv2handle {
        let h: lv2::Lv2handle;
        unsafe{ h = libc::malloc(mem::size_of::<ShimmerPlugin>() as libc::size_t); }
        let plugin: *mut ShimmerPlugin = h as *mut ShimmerPlugin;
        unsafe { (*plugin).sample_rate = sample_rate; }
        unsafe { (*plugin).shimmer_phase = 0 as f32; } // from 0-1 (1=360 degrees)
        unsafe { (*plugin).going_up = true; }
        return h;
    }

    pub extern fn connect_port(handle: lv2::Lv2handle, port: i32, data: *mut libc::c_void) {
        let plugin: *mut ShimmerPlugin = handle as *mut ShimmerPlugin;
        if port == PortIndex::Input as i32 {unsafe{ (*plugin).input = data as *const f32 }}
        else if port == PortIndex::Output as i32 {unsafe{ (*plugin).output = data as *mut f32 }}
        else if port == PortIndex::Rate as i32 {unsafe{ (*plugin).shimmer_rate = data as *const f32}}
        else if port == PortIndex::Depth as i32 {unsafe{ (*plugin).shimmer_depth = data as *const f32 }}
    }

    pub extern fn activate(_instance: lv2::Lv2handle) {}

    pub extern fn run(instance: lv2::Lv2handle, n_samples: u32) {
        let mut plugin = instance as *mut ShimmerPlugin;
        let input: *const f32 = unsafe{  (*plugin).input };
        let output: *mut f32 = unsafe{ (*plugin).output };
        let mut shimmer_rate = unsafe{ *(*plugin).shimmer_rate };
        let mut shimmer_depth = unsafe{ *(*plugin).shimmer_depth };

        if shimmer_rate < 0f32 {shimmer_rate = 0f32}
        else if shimmer_rate > 50f32 {shimmer_rate = 50f32};
        if shimmer_depth < 0f32 {shimmer_depth = 0f32}
        else if shimmer_depth > 1f32 {shimmer_depth = 1f32};

        // Really dumb triangle wave
        let mut sample: f32;
        unsafe{
            for x in 0..n_samples-1 {
                sample = *input.offset(x as isize);
                sample -= sample * shimmer_depth * (*plugin).shimmer_phase;
                *output.offset(x as isize) = sample;

                if (*plugin).going_up {
                    (*plugin).shimmer_phase += (shimmer_rate as f64 / (*plugin).sample_rate) as f32;
                    if (*plugin).shimmer_phase > 1f32 {
                        (*plugin).shimmer_phase = 1f32;
                        (*plugin).going_up = false;
                    }
                } else {
                    (*plugin).shimmer_phase -= (shimmer_rate as f64 / (*plugin).sample_rate) as f32;
                    if (*plugin).shimmer_phase < 0f32 {
                        (*plugin).shimmer_phase = 0f32;
                        (*plugin).going_up = true;
                    }
                }
            }
        }

    }

    pub extern fn deactivate(_instance: lv2::Lv2handle) {}
    pub extern fn cleanup(instance: lv2::Lv2handle) {

        unsafe{
            libc::free(instance  as lv2::Lv2handle)
        }
    }
    pub extern fn extension_data(_uri: *const u8)-> (*const libc::c_void) {
                            ptr::null()
    }
}

static mut desc: lv2::LV2Descriptor = lv2::LV2Descriptor {
    uri: 0 as *const libc::c_char, // ptr::null() isn't const fn (yet)
    instantiate: lv2::LV2Descriptor::instantiate,
    connect_port: lv2::LV2Descriptor::connect_port,
    activate: lv2::LV2Descriptor::activate,
    run: lv2::LV2Descriptor::run,
    deactivate: lv2::LV2Descriptor::deactivate,
    cleanup: lv2::LV2Descriptor::cleanup,
    extension_data: lv2::LV2Descriptor::extension_data
};


static S: &'static [u8] = b"http://www.joebutton.co.uk/software/lv2/shimmer-rust\0";
#[no_mangle]
pub extern fn lv2_descriptor(index:i32) -> *const lv2::LV2Descriptor {
    if index != 0 {
        return ptr::null();
    } else {
        // credits to ker on stackoverflow: http://stackoverflow.com/questions/31334356/static-struct-with-c-strings-for-lv2-plugin (duplicate) or http://stackoverflow.com/questions/25880043/creating-a-static-c-struct-containing-strings
        let ptr = S.as_ptr() as *const libc::c_char;
        unsafe {
            desc.uri = ptr;
            return &desc as *const lv2::LV2Descriptor
        }
    }
}
