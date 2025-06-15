// ##############################################################################
// examples/rust/slint/src/lib.rs
//
// Licensed to the Apache Software Foundation (ASF) under one or more contributor
// license agreements.  See the NOTICE file distributed with this work for
// additional information regarding copyright ownership.  The ASF licenses this
// file to you under the Apache License, Version 2.0 (the "License"); you may not
// use this file except in compliance with the License.  You may obtain a copy of
// the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
// WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
// License for the specific language governing permissions and limitations under
// the License.
//
// ##############################################################################

use std::rc::Rc;
use std::{cell::RefCell, ffi::CStr};

use slint::platform::software_renderer::{LineBufferProvider, Rgb565Pixel};

use nuttx::input::touchscreen::*;
use nuttx::video::fb::*;
use slint::platform::WindowEvent;

slint::include_modules!();

struct NuttXPlatform {
    window: Rc<slint::platform::software_renderer::MinimalSoftwareWindow>,
}

impl slint::platform::Platform for NuttXPlatform {
    fn create_window_adapter(
        &self,
    ) -> Result<Rc<dyn slint::platform::WindowAdapter>, slint::PlatformError> {
        Ok(self.window.clone())
    }
    fn duration_since_start(&self) -> std::time::Duration {
        std::time::SystemTime::now()
            .duration_since(std::time::UNIX_EPOCH)
            .unwrap()
    }
}

fn create_slint_app() -> AppWindow {
    AppWindow::new().expect("Failed to load UI")
}

#[derive(Debug, Clone)]
struct NuttXRenderer {
    frame_buffer: *mut u8,
    line_buffer: Rc<RefCell<Vec<Rgb565Pixel>>>,
    stride: usize,
    lines: usize,
}

impl LineBufferProvider for NuttXRenderer {
    type TargetPixel = Rgb565Pixel;
    fn process_line(
        &mut self,
        line: usize,
        range: std::ops::Range<usize>,
        render_fn: impl FnOnce(&mut [Self::TargetPixel]),
    ) {
        let fb_size = self.stride * self.lines;
        let frame = unsafe {
            std::slice::from_raw_parts_mut(self.frame_buffer as *mut Rgb565Pixel, fb_size)
        };
        let mut buffer = self.line_buffer.borrow_mut();
        render_fn(&mut buffer[range.clone()]);
        let offset = line * self.stride;
        frame[offset..offset + range.end - range.start].copy_from_slice(&buffer[range]);
    }
}

#[no_mangle]
pub extern "C" fn slint_main() {
    // Open the framebuffer device and get its information
    let fbdev = match FrameBuffer::new(CStr::from_bytes_with_nul(b"/dev/fb0\0").unwrap()) {
        Ok(fb) => fb,
        Err(_) => {
            println!("Failed to open framebuffer device");
            return;
        }
    };

    let planeinfo = fbdev.get_plane_info().unwrap();
    let videoinfo = fbdev.get_video_info().unwrap();

    println!("{:?}", planeinfo);
    println!("{:?}", videoinfo);

    if videoinfo.fmt != FB_FMT_RGB16_565 as u8 {
        println!("Unsupported pixel format, only RGB565 is supported for now");
        return;
    }

    // Open the touchscreen device
    let mut tsdev = match TouchScreen::open(CStr::from_bytes_with_nul(b"/dev/input0\0").unwrap()) {
        Ok(ts) => ts,
        Err(_) => {
            println!("Failed to open touchscreen device");
            return;
        }
    };

    // Setup the Slint backend
    let window = slint::platform::software_renderer::MinimalSoftwareWindow::new(Default::default());
    window.set_size(slint::PhysicalSize::new(
        videoinfo.xres.into(),
        videoinfo.yres.into(),
    ));

    // Set the platform
    slint::platform::set_platform(Box::new(NuttXPlatform {
        window: window.clone(),
    }))
    .unwrap();

    // Configure the UI
    let _ui = create_slint_app();

    // Create the renderer for NuttX
    let nxrender = NuttXRenderer {
        frame_buffer: planeinfo.fbmem as *mut u8,
        line_buffer: Rc::new(RefCell::new(vec![
            Rgb565Pixel::default();
            videoinfo.xres as usize
        ])),
        stride: videoinfo.xres.into(),
        lines: videoinfo.yres.into(),
    };

    let button = slint::platform::PointerEventButton::Left;
    let mut last_touch_down = false;
    let mut last_position = Default::default();

    loop {
        slint::platform::update_timers_and_animations();
        window.draw_if_needed(|renderer| {
            renderer.render_by_line(nxrender.clone());
        });

        if let Ok(sample) = tsdev.read_sample() {
            if sample.npoints > 0 {
                let point = sample.point[0];
                if point.is_pos_valid() {
                    let position = slint::PhysicalPosition::new(point.x.into(), point.y.into())
                        .to_logical(window.scale_factor());
                    last_position = position;
                    if point.is_touch_down() || point.is_touch_move() {
                        last_touch_down = true;
                        window.dispatch_event(WindowEvent::PointerPressed { position, button });
                    }
                }
            }
        } else {
            if last_touch_down {
                window.dispatch_event(WindowEvent::PointerReleased {
                    position: last_position,
                    button,
                });
                last_touch_down = false;
            }
        }
    }
}
