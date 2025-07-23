use gtk::glib;
use gtk::subclass::prelude::*;
use adw::subclass::prelude::*;
use crate::wrapper::pref::*;

#[derive(gtk::CompositeTemplate, Default)]
#[template(resource = "/gay/pancake/lsfg-vk/pane/main.ui")]
pub struct PaneMain {
    #[template_child]
    pub pref_multiplier: TemplateChild<PrefNumber>,
    #[template_child]
    pub pref_flow_scale: TemplateChild<PrefSlider>,
    #[template_child]
    pub pref_performance_mode: TemplateChild<PrefSwitch>,
    #[template_child]
    pub pref_hdr_mode: TemplateChild<PrefSwitch>,
    #[template_child]
    pub pref_experimental_present_mode: TemplateChild<PrefDropdown>
}

#[glib::object_subclass]
impl ObjectSubclass for PaneMain {
    const NAME: &'static str = "LSPaneMain";
    type Type = super::PaneMain;
    type ParentType = adw::NavigationPage;

    fn class_init(klass: &mut Self::Class) {
        klass.bind_template();
    }

    fn instance_init(obj: &glib::subclass::InitializingObject<Self>) {
        obj.init_template();
    }
}

impl ObjectImpl for PaneMain {
    fn constructed(&self) {
        self.parent_constructed();
    }
}

impl WidgetImpl for PaneMain {}
impl NavigationPageImpl for PaneMain {}
