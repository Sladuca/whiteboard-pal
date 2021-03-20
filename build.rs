extern crate metadeps;

fn main() {
    metadeps::probe().unwrap();
    cc::Build::new()
        .cpp(true)
        .file("src/model_fns.cpp")
        .compile("model_fns")
}