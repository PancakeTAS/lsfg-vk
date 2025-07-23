use std::cell::RefCell;

use gtk::glib;
use gtk::subclass::prelude::*;
use adw::subclass::prelude::*;
use adw::prelude::*;

#[derive(gtk::CompositeTemplate, glib::Properties, Default)]
#[properties(wrapper_type = super::PrefDropdown)]
#[template(resource = "/gay/pancake/lsfg-vk/pref/dropdown.ui")]
pub struct PrefDropdown {
    #[property(get, set)]
    opt_name: RefCell<String>,
    #[property(get, set)]
    default_selection: RefCell<u32>,
    #[property(get, set)]
    options: RefCell<gtk::StringList>,

    #[template_child]
    pub dropdown: TemplateChild<gtk::DropDown>,
}

#[glib::object_subclass]
impl ObjectSubclass for PrefDropdown {
    const NAME: &'static str = "LSPrefDropdown";
    type Type = super::PrefDropdown;
    type ParentType = adw::PreferencesRow;

    fn class_init(klass: &mut Self::Class) {
        klass.bind_template();
    }

    fn instance_init(obj: &glib::subclass::InitializingObject<Self>) {
        obj.init_template();
    }
}

#[glib::derived_properties]
impl ObjectImpl for PrefDropdown {
    fn constructed(&self) {
        self.parent_constructed();
    }
}

impl WidgetImpl for PrefDropdown {}
impl ListBoxRowImpl for PrefDropdown {}
impl PreferencesRowImpl for PrefDropdown {}
