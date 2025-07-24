use std::cell::RefCell;

use gtk::glib;
use gtk::subclass::prelude::*;
use adw::subclass::prelude::*;
use adw::prelude::*;

#[derive(gtk::CompositeTemplate, glib::Properties, Default)]
#[properties(wrapper_type = super::PrefSwitch)]
#[template(resource = "/gay/pancake/lsfg-vk/pref/switch.ui")]
pub struct PrefSwitch {
    #[property(get, set)]
    opt_name: RefCell<String>,
    #[property(get, set)]
    opt_subtitle: RefCell<String>,
    #[property(get, set)]
    default_state: RefCell<bool>,

    #[template_child]
    pub switch: TemplateChild<gtk::Switch>,
}

#[glib::object_subclass]
impl ObjectSubclass for PrefSwitch {
    const NAME: &'static str = "LSPrefSwitch";
    type Type = super::PrefSwitch;
    type ParentType = adw::PreferencesRow;

    fn class_init(klass: &mut Self::Class) {
        klass.bind_template();
    }

    fn instance_init(obj: &glib::subclass::InitializingObject<Self>) {
        obj.init_template();
    }
}

#[glib::derived_properties]
impl ObjectImpl for PrefSwitch {
    fn constructed(&self) {
        self.parent_constructed();
    }
}

impl WidgetImpl for PrefSwitch {}
impl ListBoxRowImpl for PrefSwitch {}
impl PreferencesRowImpl for PrefSwitch {}
