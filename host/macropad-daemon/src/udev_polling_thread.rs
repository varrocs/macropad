use super::events::Messages;
use mio::net::UnixStream;
use mio::{Events, Interest, Poll, Token};
use std::io;
use std::os::unix::io::{AsRawFd, FromRawFd};
use std::path::PathBuf;
use std::sync::mpsc::Sender;

const ID_VENDOR_ID: &str = "0483";
const ID_MODEL_ID: &str = "5740";

fn enumerate_devices() -> io::Result<Option<PathBuf>> {
    let mut enumerator = udev::Enumerator::new().unwrap();
    enumerator.match_property("ID_VENDOR_ID", ID_VENDOR_ID)?;
    enumerator.match_property("ID_MODEL_ID", ID_MODEL_ID)?;
    enumerator.match_subsystem("tty")?;

    let device_list = enumerator.scan_devices()?;

    if let Some(dev) = device_list.into_iter().next() {
        return Ok(dev.devnode().map(|p| p.to_path_buf()));
    }
    Ok(None)
}

fn is_macropad_event(event: &udev::Event) -> bool {
    let vendor_id = event.property_value("ID_VENDOR_ID");
    let model_id = event.property_value("ID_MODEL_ID");
    vendor_id.is_some()
        && model_id.is_some()
        && vendor_id.unwrap() == ID_VENDOR_ID
        && model_id.unwrap() == ID_MODEL_ID
}

pub fn udev_thread(channel: Sender<Messages>) -> io::Result<()> {
    log::info!("udev - Enumerate devices");
    let found_dev = enumerate_devices()?;
    match found_dev {
        Some(dev) => {
            log::info!("Found device: {}", dev.display());
            channel.send(Messages::MacropadDetected(dev));
        }
        None => {
            log::info!("Couldn't find device");
        }
    }

    log::info!("udev - Setting up monitoring");

    let mut monitor_socket = udev::MonitorBuilder::new()?
        .match_subsystem("tty")?
        .listen()?;
    let mut unix_stream = unsafe { UnixStream::from_raw_fd(monitor_socket.as_raw_fd()) };
    let mut poll = Poll::new()?;

    poll.registry()
        .register(&mut unix_stream, Token(0), Interest::READABLE)?;

    let mut events = Events::with_capacity(1024);
    log::info!("udev - Starting monitoring");
    loop {
        poll.poll(&mut events, None)?;

        while let Some(event) = monitor_socket.next() {
            if is_macropad_event(&event) {
                match event.event_type() {
                    udev::EventType::Add => {
                        if let Some(node) = event.devnode() {
                            log::info!("udev - Found added device, {:?}", node);
                            channel.send(Messages::MacropadDetected(node.to_path_buf()));
                        }
                    }
                    udev::EventType::Remove => {
                        channel.send(Messages::MacropadUnplugged);
                    }
                    _ => {
                        log::debug!("Unknown macropad event: {}", event.event_type());
                    }
                }
            }
        }
    }
}
