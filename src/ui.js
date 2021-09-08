// Brian Chrzanowski
// 2021-09-07 01:17:08

const email_re = /^(([^<>()[\]\\.,;:\s@\"]+(\.[^<>()[\]\\.,;:\s@\"]+)*)|(\".+\"))@((\[[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\])|(([a-zA-Z\-0-9]+\.)+[a-zA-Z]{2,}))$/;

const COOKIE_DISCLAIMER = ` For legal reasons, you need to accept the fact that this site requires 1
session cookie for all operations where you might create new content.  It is impossible to opt-out
of this cookie; however, you can still view recipes to your heart's content without using cookies.
`;

// MenuComponent
function MenuComponent(initialVnode) {
    return {
        view: function(vnode) {
            return m("nav", [
                m(m.route.Link, { href: "/" }, "Home"),
                m(m.route.Link, { href: "/recipe" }, "New Recipe"),
                m(m.route.Link, { href: "/search" }, "Search"),
            ]);
        }
    };
}

// Recipe: the Recipe data structure
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

// RecipeComponent
function RecipeComponent(initialVnode) {
    return {
        view: function(vnode) {

            return [
                m(MenuComponent),
                m("h3", "Recipe Page"),
            ];
        }
    }
};

// SearchComponent: 
function SearchComponent(initialVnode) {
    return {
        view: function(vnode) {
            return [
                m(MenuComponent),
                m("h3", "Search Page"),
            ];
        }
    }
}

// HomeComponent
function HomeComponent(initialVnode) {
    var count = 0;

    return {
        view: function(vnode) {
            return m("main", [
                m(MenuComponent),
                m("h1", { class: "title" }, "Recipe Website"),
                m("p", "This is the recipe website"),
                m("button", { onclick: function() { count++; }}, count + " clicks"),
                m("br"),
                m("button", {
                    onclick: function () { m.route.set("/newuser"); }
                }, "New User Page!"),
                m("br"),
                m("button", {
                    onclick: function () { m.route.set("/login"); }
                }, "Login!"),
            ]);
        }
    };
}

// SuccessComponent
function SuccessComponent(initialVnode) {
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

// ErrorComponent
function ErrorComponent(initialVnode) {
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

// LoginComponent
function LoginComponent(inivialVnode) {
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
                    m("h2", "Login"),

                    m("div", [

                        m("div", [
                            m("label", "Username"),
                            m("input[type=text]", {
                                oninput: function (e) { auth.setUsername(e.target.value); },
                                value: auth.username,
                            }),
                        ]),

                        m("div", [
                            m("label", "Password"),
                            m("input[type=password,autocomplete=off]", {
                                oninput: function (e) { auth.setPassword(e.target.value); },
                                value: auth.password,
                            }),
                        ]),

                        m("div", [
                            m("button.button[type=submit]", "Login"),
                            m("button.button[type=button]", {
                                onclick: function() { m.route.set("/home"); }
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
function NewUserComponent(initialVnode) {
    const auth = new Auth();

    return {
        view: function(vnode) {
            return m(".newuser", [
                m("form", {
                    onsubmit: function (e) {
                        e.preventDefault();
                        console.log("submitted the form");
                    },
                }, [
                    m("h2", "New User"),

                    m("div", [

                        m("div", [
                            m("label", "Username"),
                            m("input[type=text]", {
                                oninput: function (e) { auth.setUsername(e.target.value); },
                                value: auth.username,
                            }),
                        ]),

                        m("div", [
                            m("label", "Email"),
                            m("input[type=email]", {
                                oninput: function (e) { auth.setEmail(e.target.value); },
                                value: auth.email,
                            }),
                        ]),

                        m("div", [
                            m("label", "Password"),
                            m("input[type=password,autocomplete=off]", {
                                oninput: function (e) { auth.setPassword(e.target.value); },
                                value: auth.password,
                            }),
                        ]),

                        m("div", [
                            m("label", "Verify Password"),
                            m("input[type=password]", {
                                oninput: function (e) { auth.setVerify(e.target.value); },
                                value: auth.verify,
                            }),
                        ]),

                        m("div", [
                            m("button.button[type=submit]", "Save"),
                            m("button.button[type=button]", {
                                onclick: function() { m.route.set("/home"); },
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

const routes = {
    "/": HomeComponent,
    "/newuser": NewUserComponent,
    "/search": SearchComponent,
    "/login": LoginComponent,
    "/recipe/:id": RecipeComponent,
};

m.route(root, "/", routes);

