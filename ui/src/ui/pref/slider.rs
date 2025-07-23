use std::cell::RefCell;

use gtk::glib;
use gtk::subclass::prelude::*;
use adw::subclass::prelude::*;
use adw::prelude::*;

#[derive(gtk::CompositeTemplate, glib::Properties, Default)]
#[properties(wrapper_type = super::PrefSlider)]
#[template(resource = "/gay/pancake/lsfg-vk/pref/slider.ui")]
pub struct PrefSlider {
    #[property(get, set)]
    opt_name: RefCell<String>,

    #[template_child]
    pub slider: TemplateChild<gtk::Scale>,
}

#[glib::object_subclass]
impl ObjectSubclass for PrefSlider {
    const NAME: &'static str = "LSPrefSlider";
    type Type = super::PrefSlider;
    type ParentType = adw::PreferencesRow;

    fn class_init(klass: &mut Self::Class) {
        klass.bind_template();
    }

    fn instance_init(obj: &glib::subclass::InitializingObject<Self>) {
        obj.init_template();
    }
}

#[glib::derived_properties]
impl ObjectImpl for PrefSlider {
    fn constructed(&self) {
        self.parent_constructed();
    }
}

impl WidgetImpl for PrefSlider {}
impl ListBoxRowImpl for PrefSlider {}
impl PreferencesRowImpl for PrefSlider {}
