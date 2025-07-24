use std::cell::RefCell;

use gtk::glib;
use gtk::subclass::prelude::*;
use adw::subclass::prelude::*;
use adw::prelude::*;

#[derive(gtk::CompositeTemplate, glib::Properties, Default)]
#[properties(wrapper_type = super::PrefNumber)]
#[template(resource = "/gay/pancake/lsfg-vk/pref/number.ui")]
pub struct PrefNumber {
    #[property(get, set)]
    opt_name: RefCell<String>,
    #[property(get, set)]
    opt_subtitle: RefCell<String>,

    #[template_child]
    pub number: TemplateChild<gtk::SpinButton>,
}

#[glib::object_subclass]
impl ObjectSubclass for PrefNumber {
    const NAME: &'static str = "LSPrefNumber";
    type Type = super::PrefNumber;
    type ParentType = adw::PreferencesRow;

    fn class_init(klass: &mut Self::Class) {
        klass.bind_template();
    }

    fn instance_init(obj: &glib::subclass::InitializingObject<Self>) {
        obj.init_template();
    }
}

#[glib::derived_properties]
impl ObjectImpl for PrefNumber {
    fn constructed(&self) {
        self.parent_constructed();
    }
}

impl WidgetImpl for PrefNumber {}
impl ListBoxRowImpl for PrefNumber {}
impl PreferencesRowImpl for PrefNumber {}
