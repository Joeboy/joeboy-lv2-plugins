/******************************************
 * Vague effort at a rust version of lv2.h
 ******************************************/

extern crate libc;

pub type Lv2handle = *mut libc::c_void;

#[repr(C)]
pub struct LV2Feature {
    uri: *const u8,
    data: *mut libc::c_void
}

#[repr(C)]
pub struct LV2Descriptor {
    pub uri: *const  libc::c_char,
    pub instantiate: extern fn(descriptor: *const LV2Descriptor, rate: f64,
                            bundle_path: *const u8, features: *const LV2Feature)
                                -> Lv2handle,
    pub connect_port: extern fn(handle: Lv2handle, port: i32, data: *mut libc::c_void),
    pub activate: extern fn(instance: Lv2handle),
    pub run: extern fn(instance: Lv2handle, n_samples: u32),
    pub deactivate: extern fn(instance: Lv2handle),
    pub cleanup: extern fn(instance: Lv2handle),
    pub extension_data: extern fn(uri: *const u8)-> (*const libc::c_void),
}
