use super::config::Config;
use super::events::Messages;
use std::io;
use std::io::BufRead;
use std::io::Write;
use std::os::unix::net::{UnixListener, UnixStream};
use std::sync::mpsc::Sender;
use std::sync::{Arc, RwLock};
use std::thread;

fn handle_client(config: Arc<RwLock<Config>>, channel: Sender<Messages>, mut stream: UnixStream) {
    let reader = std::io::BufReader::new(stream.try_clone().unwrap());
    for line in reader.lines() {
        match line {
            Err(_) => {
                return;
            }
            Ok(msg) => {
                log::debug!("Got line {msg}");
                let response = if msg.starts_with("get_layout") {
                    let config = config.read().unwrap();
                    config.current_layout.clone()
                } else if msg.starts_with("list_layout") {
                    let config = config.read().unwrap();
                    config.list_layout_names().join("\n")
                } else if msg.starts_with("set_layout") {
                    msg.split(" ")
                        .nth(1)
                        .ok_or("Missing parameter")
                        .and_then(|layout| {
                            let mut config = config.write().unwrap();
                            let response = config.set_current_layout(layout);
                            channel.send(Messages::ConfigChanged);
                            response
                        })
                        .err()
                        .unwrap_or("Ok")
                        .to_string()
                } else {
                    "Uknown command".to_string()
                };
                stream
                    .write_all(response.as_bytes())
                    .and_then(|_| stream.flush())
                    .expect("Failed to write response");
                break;
            }
        }
    }
}

pub fn cli_server_thread(
    config: Arc<RwLock<Config>>,
    channel: Sender<Messages>,
    socket_path: &str,
) -> io::Result<()> {
    let listener = UnixListener::bind(socket_path)?;
    log::info!("Listening on {socket_path}");

    // accept connections and process them, spawning a new thread for each one
    for stream in listener.incoming() {
        match stream {
            Ok(stream) => {
                /* connection succeeded */
                log::info!("CLI client connected");
                let c = config.clone();
                let s = channel.clone();
                thread::spawn(move || handle_client(c, s, stream));
            }
            Err(err) => {
                /* connection failed */
                log::error!("CLI server error: {err}");
                break;
            }
        }
    }
    log::info!("CLI Server exiting");
    Ok(())
}
