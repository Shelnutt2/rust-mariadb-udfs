#![crate_type = "staticlib"]
extern crate libc;
extern crate rayon;

use rayon::prelude::*;


// #[no_mangle]
// pub extern "C" fn rust_sum_int(size: libc::size_t,
//                                array_pointer: *const libc::int32_t)
//                                -> libc::int32_t {
//     return internal_rust_sum(unsafe {
//         std::slice::from_raw_parts(array_pointer as *const i32, size as usize)
//     }) as libc::int32_t;
// }

#[no_mangle]
pub extern "C" fn rust_sum_float(array_pointer: *const libc::c_double,
                                 size: libc::size_t)
                                 -> libc::c_double {
    return internal_rust_sum(unsafe {
        std::slice::from_raw_parts(array_pointer as *const f64, size as usize)
    }) as libc::c_double;
}



fn internal_rust_sum(array: &[f64]) -> f64 {
    assert!(!array.is_empty());
    return array.into_par_iter().map(|&a| a).sum();
}
