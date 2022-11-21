use super::config::Config;
use super::program_device::setup_device;
use std::path::PathBuf;
use std::sync::mpsc::Receiver;
use std::sync::{Arc, RwLock};

use super::events::Messages;

fn try_setup_device(conf: &Arc<RwLock<Config>>, path: &Option<PathBuf>) {
    if path.is_none() {
        return;
    }
    let path = path.as_ref().unwrap();
    let conf = conf.read().unwrap();
    let layout = conf.get_current_layout();
    if let Some(layout) = layout {
        let res = setup_device(layout, path);
        if res.is_err() {
            log::warn!("Failed to set up layout, error: {}", res.unwrap_err());
        }
    } else {
        log::warn!("Can't get the current layout, can't program the device");
    }
}

pub fn programmer_thread_func(config: Arc<RwLock<Config>>, channel: Receiver<Messages>) -> ! {
    let mut current_dev_path: Option<PathBuf> = None;
    let current_config = config;

    loop {
        let message: Messages = channel.recv().unwrap();

        match message {
            Messages::MacropadDetected(path) => {
                current_dev_path = Some(path);
            }
            Messages::MacropadUnplugged => {
                current_dev_path = None;
            }
            Messages::ConfigChanged => {
                try_setup_device(&current_config, &current_dev_path);
            }
        };
    }
}
