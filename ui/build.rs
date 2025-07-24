fn main() {
    glib_build_tools::compile_resources(
        &["rsc"],
        "rsc/lsfg-vk.xml",
        "lsfg-vk.gresource",
    );
}
