use gtk::{prelude::RangeExt, subclass::prelude::*};
use adw::subclass::prelude::*;
use crate::ui::pref::*;
use gtk::{glib, CompositeTemplate};

#[derive(CompositeTemplate, Default)]
#[template(resource = "/gay/pancake/lsfg-vk/window.ui")]
pub struct Window {
    // main config elements
    #[template_child]
    pref_multiplier: TemplateChild<PrefNumber>,
    #[template_child]
    pref_flow_scale: TemplateChild<PrefSlider>,
    #[template_child]
    pref_performance_mode: TemplateChild<PrefSwitch>,
    #[template_child]
    pref_hdr_mode: TemplateChild<PrefSwitch>,
    #[template_child]
    pref_experimental_present_mode: TemplateChild<PrefDropdown>,
}

#[glib::object_subclass]
impl ObjectSubclass for Window {
    const NAME: &'static str = "LSApplicationWindow";
    type Type = super::Window;
    type ParentType = adw::ApplicationWindow;

    fn class_init(klass: &mut Self::Class) {
        klass.bind_template();
    }

    fn instance_init(obj: &glib::subclass::InitializingObject<Self>) {
        obj.init_template();
    }
}

impl ObjectImpl for Window {
    fn constructed(&self) {
        self.parent_constructed();

        self.pref_multiplier.get()
            .imp().number.get().connect_value_changed(|dropdown| {
                let selected = dropdown.value();
                println!("Multiplier changed: {}", selected);
            });
        self.pref_flow_scale.get()
            .imp().slider.get().connect_value_changed(|slider| {
                let value = slider.value();
                println!("Flow scale changed: {}", value);
            });
        self.pref_performance_mode.get()
            .imp().switch.get().connect_state_notify(|switch| {
                let state = switch.state();
                println!("Performance mode changed: {}", state);
            });
        self.pref_hdr_mode.get()
            .imp().switch.get().connect_state_notify(|switch| {
                let state = switch.state();
                println!("HDR mode changed: {}", state);
            });
        self.pref_experimental_present_mode.get()
            .imp().dropdown.get().connect_selected_notify(|dropdown| {
                let selected = dropdown.selected();
                println!("Experimental present mode changed: {}", selected);
            });
    }
}

impl WidgetImpl for Window {}
impl WindowImpl for Window {}
impl ApplicationWindowImpl for Window {}
impl AdwWindowImpl for Window {}
impl AdwApplicationWindowImpl for Window {}
