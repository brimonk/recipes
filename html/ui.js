// Brian Chrzanowski
// 2021-09-01 19:40:45
//
// The Recipe Website's UI Code

const email_re = /^(([^<>()[\]\\.,;:\s@\"]+(\.[^<>()[\]\\.,;:\s@\"]+)*)|(\".+\"))@((\[[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\])|(([a-zA-Z\-0-9]+\.)+[a-zA-Z]{2,}))$/;

const CARD_CLASS = "bg-white w-full shadow-md rounded px-8 pt-6 pb-8 mb-4";
const LABEL_CLASS = "block uppercase tracking-wide text-gray-700 text-xs font-bold mb-2";
const INPUT_CLASS = "appearance-none block w-full bg-gray-200 text-gray-700 border border-gray-200 rounded py-3 px-4 mb-3 leading-tight focus:outline-none focus:bg-white focus:border-gray-500";
const PRIMARY_BUTTON_CLASS = "bg-blue-500 hover:bg-blue-400 text-white font-bold py-2 px-4 rounded";
const SECONDARY_BUTTON_CLASS = "bg-gray-500 hover:bg-gray-400 text-white font-bold py-2 px-4 rounded";
const DANGER_BUTTON_CLASS = "bg-red-500 hover:bg-gray-400 text-white font-bold py-2 px-4 rounded";

// Recipe Component

// Search Component

// Home Component
function Home(initialVnode) {
    var count = 0;

    return {
        view: function(vnode) {
            return m("main", [
                m("h1", { class: "title" }, "Recipe Website"),
                m("p", "This is the recipe website"),
                m("button", { onclick: function() { count++; }}, count + " clicks"),
                m("a", { href: "#!/newuser" }, "New User Page!"),
            ]);
        }
    };
}

// Success Component

// Error Component

// Auth Object
class Auth {
    constructor() {
        this.username = "";
        this.password = "";
        this.email = "";
        this.verify = "";

        this.allRequired = false;
    }

    setUsername(value) {
        this.username = value;
    }

    setPassword(value) {
        this.password = value;
    }

    setEmail(value) {
        this.email = value;
    }

    setVerify(value) {
        this.verify = value;
    }

    setAllRequired(value) {
        this.allRequired = value;
    }

    canSubmit() {
        // TODO (Brian) add in validation logic
        return true;
    }

    login() {
        // TODO (Brian) add login logic
    }
}

// Login Component

// New User Component
function NewUser(initialVnode) {
    const auth = new Auth();

    return {
        view: function(vnode) {
            return m(".newuser", [
                m("form", {
                    class: "container mx-auto px-2 " + CARD_CLASS,
                    onsubmit: function (e) {
                        e.preventDefault();
                        console.log("submitted the form");
                    },
                }, [
                m("h2", { class: "text-lg mb-4" }, "New User"),

                m("div", { class: "flex flex-wrap -mx-3 mb-4" }, [

                    m("div", { class: "w-full px-3" }, [
                        m("label", { class: LABEL_CLASS }, "Username"),
                        m("input[type=text]", {
                            class: INPUT_CLASS,
                            oninput: function (e) { auth.setUsername(e.target.value); },
                            value: auth.username,
                        }),
                    ]),

                    m("div", { class: "w-full px-3" }, [
                        m("label", { class: LABEL_CLASS }, "Email"),
                        m("input[type=email]", {
                            class: INPUT_CLASS,
                            oninput: function (e) { auth.setEmail(e.target.value); },
                            value: auth.email,
                        }),
                    ]),

                    m("div", { class: "w-full px-3" }, [
                        m("label", { class: LABEL_CLASS }, "Password"),
                        m("input[type=password,autocomplete=off]", {
                            class: INPUT_CLASS,
                            oninput: function (e) { auth.setPassword(e.target.value); },
                            value: auth.password,
                        }),
                    ]),

                    m("div", { class: "w-full px-3" }, [
                        m("label", { class: LABEL_CLASS }, "Verify Password"),
                        m("input[type=password]", {
                            class: INPUT_CLASS,
                            oninput: function (e) { auth.setVerify(e.target.value); },
                            value: auth.verify,
                        }),
                    ]),

                    m("div", { class: "flex flex-wrap -mx-3 w-full px-3" }, [
                        m("button.button[type=submit]", { class: PRIMARY_BUTTON_CLASS }, "Save"),
                        m("a[type=button]", { href: "#!/home", class: SECONDARY_BUTTON_CLASS  }, "Cancel"),
                    ]),
                ]),

                ]),
            ]);
        },
    };
}

// hook it all up
var root = document.body;

m.route(root, "/home", {
    "/home": Home,
    "/newuser": NewUser,
});

// m.mount(root, Home);

// m.render(document.body, "hello world");

