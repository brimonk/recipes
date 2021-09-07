// Brian Chrzanowski
// 2021-09-07 01:17:08

import m from "mithril";

const email_re = /^(([^<>()[\]\\.,;:\s@\"]+(\.[^<>()[\]\\.,;:\s@\"]+)*)|(\".+\"))@((\[[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\])|(([a-zA-Z\-0-9]+\.)+[a-zA-Z]{2,}))$/;

const CARD_CLASS = "bg-white w-full shadow-md rounded px-8 pt-6 pb-8 mb-4";
const LABEL_CLASS = "block uppercase tracking-wide text-gray-700 text-xs font-bold mb-2";
const INPUT_CLASS = "appearance-none block w-full bg-gray-200 text-gray-700 border border-gray-200 rounded py-3 px-4 mb-3 leading-tight focus:outline-none focus:bg-white focus:border-gray-500";
const PRIMARY_BUTTON_CLASS = "bg-blue-500 hover:bg-blue-400 text-white font-bold py-2 px-4 rounded";
const SECONDARY_BUTTON_CLASS = "bg-gray-500 hover:bg-gray-400 text-white font-bold py-2 px-4 rounded";
const DANGER_BUTTON_CLASS = "bg-red-500 hover:bg-gray-400 text-white font-bold py-2 px-4 rounded";

const COOKIE_DISCLAIMER = ` For legal reasons, you need to accept the fact that this site requires 1
session cookie for all operations where you might create new content.  It is impossible to opt-out
of this cookie; however, you can still view recipes to your heart's content without using cookies.
`;

var MenuComponent = {
    view: function() {
        return m("nav", [
            m(m.route.Link, { href: "/" }, "Home"),
            m(m.route.Link, { href: "/newrecipe" }, "New Recipe"),
            m(m.route.Link, { href: "/search" }, "Search"),
        ]);
    },
};

class Recipe {
    constructor() {
        this.name = "";

        this.cook_time = -1;
        this.prep_time = -1;

        this.notes = "";

        this.ingredients = [];
        this.steps = [];
        this.tags = [];
    }

    // isValid: returns true if the object is valid for upserting
    isValid() {
        return true;
    }
}

// Recipe Component
function RecipeComponent(initialVnode) {
    return [
        m(MenuComponent),
        m("h3", "Recipe Page"),
    ];
};

// SearchComponent: 
function SearchComponent(initialVnode) {
    return [
        m(MenuCompnent),
        m("h3", "Search Page"),
    ];
}

// Home Component
function Home(initialVnode) {
    var count = 0;

    return {
        view: function(vnode) {
            return m("main", [
                m("h1", { class: "title" }, "Recipe Website"),
                m("p", "This is the recipe website"),
                m("button", { onclick: function() { count++; }}, count + " clicks"),
                m("br"),
                m("a", {
                    onclick: function () { m.route.set("/newuser"); }
                }, "New User Page!"),
                m("br"),
                m("a", {
                    onclick: function () { m.route.set("/login"); }
                }, "Login!"),
            ]);
        }
    };
}

// Success Component
function Success(initialVnode) {
    const { message, next, timeout } = initialVnode.attrs;

    setTimeout(() => {
        m.route.set(next);
    }, timeout);

    return {
        view: function(vnode) {
            return m("div", [
                m("h3", "Success!"),
                m("p", message),
            ]);
        }
    };
}

// Error Component
function Error(initialVnode) {
    const { message, next, timeout } = initialVnode.attrs;

    setTimeout(() => {
        m.route.set(next);
    }, timeout);

    return {
        view: function(vnode) {
            return m("div", [
                m("h3", "Error!"),
                m("p", { style: "color=red" }, message),
            ]);
        }
    };
}

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

    isValid() {
        // TODO (Brian) add in validation logic
        return true;
    }

    login() {
        // TODO (Brian) add login logic
    }
}

// Login Component
function Login(inivialVnode) {
    const auth = new Auth();

    return {
        view: function(vnode) {
            return m(".login", [
                m("form", {
                    class: "container mx-auto px-2 " + CARD_CLASS,
                    onsubmit: function (e) {
                        e.preventDefault();
                        console.log("submitted the form");
                    },
                }, [
                    m("h2", { class: "text-lg mb-4" }, "Login"),

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
                            m("label", { class: LABEL_CLASS }, "Password"),
                            m("input[type=password,autocomplete=off]", {
                                class: INPUT_CLASS,
                                oninput: function (e) { auth.setPassword(e.target.value); },
                                value: auth.password,
                            }),
                        ]),

                        m("div", { class: "flex flex-wrap -mx-3 w-full px-3" }, [
                            m("button.button[type=submit]", { class: PRIMARY_BUTTON_CLASS }, "Login"),
                            m("button.button[type=button]", {
                                class: SECONDARY_BUTTON_CLASS,
                                onclick: function() { m.route.set("/#!/home"); }
                            }, "Cancel"),
                        ]),
                    ]),

                    m("p", COOKIE_DISCLAIMER),

                ]),
            ]);

        },
    };
}

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
                            m("a[type=button]", {
                                onclick: function() { m.route.set("/home"); },
                                class: SECONDARY_BUTTON_CLASS
                            }, "Cancel"),
                        ]),
                    ]),

                    m("p", COOKIE_DISCLAIMER),

                ]),
            ]);
        },
    };
}

// hook it all up
var root = document.body;

m.route(root, "/", {
    "/": Home,
    "/newuser": NewUser,
    "/login": Login,
    "/recipe/:id": RecipeComponent,
});

// m.render(root, "Testing World!");

