mod cli_server_thread;
mod config;
mod events;
mod inotify_thread;
mod program_device;
mod programmer_thread;
mod udev_polling_thread;

use cli_server_thread::cli_server_thread;
use events::Messages;
use inotify_thread::inotify_thread_func;
use programmer_thread::programmer_thread_func;
use simple_logger::SimpleLogger;
use std::fs;
use std::io;
use std::sync::mpsc::channel;
use std::sync::{Arc, RwLock};
use std::thread;
use udev_polling_thread::udev_thread;

const CONFIG_FILE: &str = "config.yaml";
const SOCKET_FILE: &str = "/home/varrocs/macropad_daemon.sock";

fn handle_exit() {
    log::info!("received Ctrl+C!, Deleting socket file");
    match fs::remove_file(SOCKET_FILE) {
        Ok(_) => log::info!("Bye"),
        Err(e) => log::error!("failed to delete {SOCKET_FILE}, {}", e),
    }
    std::process::exit(0);
}

fn main() -> io::Result<()> {
    SimpleLogger::new().init().unwrap();
    ctrlc::set_handler(handle_exit).expect("Error setting Ctrl-C handler");

    log::info!("Loading config: {CONFIG_FILE}");
    let conf = fs::read_to_string(CONFIG_FILE)?;
    let conf =
        config::load_config(&conf).map_err(|msg| io::Error::new(io::ErrorKind::Other, msg))?;
    log::info!("Config loaded. There are {} layouts", conf.layouts.len());

    let conf = Arc::new(RwLock::new(conf));
    let (msg_sender, msg_receiver) = channel::<Messages>();

    let thread_handle = {
        let sender = msg_sender.clone();
        thread::spawn(move || {
            udev_thread(sender).expect("Failed to run udev thread");
        })
    };

    let thread_handle = {
        let conf = conf.clone();
        let sender = msg_sender.clone();
        thread::spawn(move || {
            cli_server_thread(conf, sender, SOCKET_FILE)
                .map_err(|err| {
                    log::error!("Failed to run cli server thread: {err}");
                    Ok::<(), io::Error>(())
                })
                .unwrap();
        })
    };

    let programmer_thread = {
        let conf = conf.clone();
        thread::spawn(move || {
            programmer_thread_func(conf, msg_receiver);
        })
    };

    let inotify_thread = {
        let conf = conf.clone();
        let sender = msg_sender.clone();
        thread::spawn(move || {
            inotify_thread_func(CONFIG_FILE, conf, sender);
        })
    };

    thread_handle.join().expect("Failed to join thread");
    Ok(())
}
