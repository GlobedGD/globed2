fn main() {
    // Trigger recompilation when a new migration is added
    println!("cargo:rerun-if-changed=migrations");
}
