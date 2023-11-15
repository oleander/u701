use glob::glob;
use std::path::PathBuf;
use std::env;
use std::io::{self, Write};

macro_rules! debug {
  ($($arg:tt)*) => {
    // if cfg!(debug_assertions) {
      writeln!(io::stderr(), $($arg)*).unwrap();
    // }
  }
}

fn main() -> Result<(), Box<dyn std::error::Error>> {
  embuild::build::CfgArgs::output_propagated("ESP_IDF")?;
  embuild::build::LinkArgs::output_propagated("ESP_IDF")?;

  // let env = "release";

  // println!("cargo:rustc-link-lib=bzip2");

  // let headers = PathBuf::from(env::var("CARGO_MANIFEST_DIR").unwrap())
  //   .join(".pio")
  //   .join("libdeps")
  //   .join("release")
  //   .join("*")
  //   .join("{include,src}")
  //   .join("*.h");


  // let entries = glob(".pio/libdeps/release/*/include/*.h")?
  //   .chain(glob(".pio/libdeps/release/*/src/*.h")?);

  // let pretty_entries: Vec<_> = entries.map(|e| e.unwrap().to_str().unwrap().to_string()).collect();

  // panic!("All entries: {:?}", pretty_entries);


  // print using panic! if CARGO_PIO_BUILD_BINDGEN_RUN is defined
  // if env::var("CARGO_PIO_BUILD_BINDGEN_EXTRA_CLANG_ARGS").is_ok() {
  //   panic!("CARGO_PIO_BUILD_BINDGEN_EXTRA_CLANG_ARGS is defined: {:?}", env::var("CARGO_PIO_BUILD_BINDGEN_EXTRA_CLANG_ARGS"));
  // }

  // for entry in entries {
  //   match entry {
  //     Ok(path) => {
  //       debug!("\tGenerating bindings for: {:?}", path);
  //       let bindings =
  //         bindgen::Builder::default().header(path.to_str().unwrap()).generate().expect("Unable to generate bindings");

  //       let out_path = PathBuf::from(env::var("OUT_DIR").unwrap());
  //       bindings
  //         .write_to_file(out_path.join(format!("{}_bindings.rs", path.file_stem().unwrap().to_str().unwrap())))
  //         .expect("Couldn't write bindings!");
  //       debug!("\tBindings generated");
  //     },
  //     Err(e) => debug!("{:?}", e)
  //   }
  // }

  Ok(())
}
