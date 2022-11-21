// macropad layout get
// macropad layout set <name>
// macropad key <key_num> get
// macropad key <key_num> set <seq>
// macropad reload

use clap::{arg, Command};
use std::io;
use std::io::prelude::*;
use std::os::unix::net::UnixStream;

const SOCKET_FILE: &str = "/home/varrocs/macropad_daemon.sock";

fn cli() -> Command {
    Command::new("macropad_cli")
        .subcommand_required(true)
        .subcommand(
            Command::new("layout")
                .about("Get / set the layout")
                .subcommand_required(true)
                .subcommand(Command::new("list"))
                .subcommand(Command::new("get"))
                .subcommand(Command::new("set").arg(arg!(<layout>))),
        )
        .subcommand(Command::new("reload"))
}

fn send_and_wait(msg: &[u8]) -> io::Result<String> {
    let mut conn = UnixStream::connect(SOCKET_FILE)?;
    // println!("Writing '{:?}'", msg);
    conn.write_all(msg)?;
    conn.flush()?;
    let mut response = String::new();
    conn.read_to_string(&mut response)?;
    // println!("Got reponse '{response}'");
    Ok(response)
}

fn main() {
    let option_err = Err(io::Error::new(io::ErrorKind::Other, "oh no"));
    let matches = cli().get_matches();
    let response = if let Some(matches) = matches.subcommand_matches("layout") {
        if let Some(_) = matches.subcommand_matches("get") {
            send_and_wait(b"get_layout\n")
        } else if let Some(_) = matches.subcommand_matches("list") {
            send_and_wait(b"list_layout\n")
        } else if let Some(matches) = matches.subcommand_matches("set") {
            if let Some(layout) = matches.get_one::<String>("layout") {
                let mut v = Vec::<u8>::new();
                write!(&mut v, "set_layout {}\n", layout).unwrap();
                send_and_wait(&v)
            } else {
                option_err
            }
        } else {
            option_err
        }
    } else if let Some(_) = matches.subcommand_matches("reload") {
        send_and_wait(b"reload\n")
    } else {
        option_err
    };
    match response {
        Ok(msg) => println!("{msg}"),
        Err(err) => eprintln!("Error: {err}"),
    }
}
