use super::config;
use super::config::Config;
use super::events::Messages;
use inotify::{Inotify, WatchMask};
use std::fs;
use std::io;
use std::sync::mpsc::Sender;
use std::sync::{Arc, RwLock};

fn load_config(config_path: &str) -> io::Result<Config> {
    let conf = fs::read_to_string(config_path)?;
    let conf =
        config::load_config(&conf).map_err(|msg| io::Error::new(io::ErrorKind::Other, msg))?;
    log::info!("Config re-loaded. There are {} layouts", conf.layouts.len());

    Ok(conf)
}

pub fn inotify_thread_func(
    config_path: &str,
    config: Arc<RwLock<Config>>,
    channel: Sender<Messages>,
) -> ! {
    log::info!("Setup watching {config_path}");
    let mut inotify = Inotify::init().expect("Failed to initialize an inotify instance");
    let mut buffer = [0; 4096];

    log::info!("Start watching {config_path}");
    loop {
        log::debug!("Reading events");

        inotify
            .add_watch(config_path, WatchMask::MODIFY)
            .expect("Failed to add file watch");

        let events = inotify
            .read_events_blocking(&mut buffer)
            .expect("Error while waiting for events");

        for event in events {
            log::debug!("{:?}", event);
        }

        let new_conf = load_config(config_path).unwrap();

        let mut guard = config.write().unwrap();
        *guard = new_conf;

        channel.send(Messages::ConfigChanged);
    }
}
