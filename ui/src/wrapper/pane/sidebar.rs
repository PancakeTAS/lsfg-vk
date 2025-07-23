use gtk::glib;
use gtk::subclass::prelude::*;
use adw::subclass::prelude::*;

#[derive(gtk::CompositeTemplate, Default)]
#[template(resource = "/gay/pancake/lsfg-vk/pane/sidebar.ui")]
pub struct PaneSidebar {

}

#[glib::object_subclass]
impl ObjectSubclass for PaneSidebar {
    const NAME: &'static str = "LSPaneSidebar";
    type Type = super::PaneSidebar;
    type ParentType = adw::NavigationPage;

    fn class_init(klass: &mut Self::Class) {
        klass.bind_template();
    }

    fn instance_init(obj: &glib::subclass::InitializingObject<Self>) {
        obj.init_template();
    }
}

impl ObjectImpl for PaneSidebar {
    fn constructed(&self) {
        self.parent_constructed();
    }
}

impl WidgetImpl for PaneSidebar {}
impl NavigationPageImpl for PaneSidebar {}
