// Brian Chrzanowski
// 2021-09-07 01:17:08

const email_re = /^(([^<>()[\]\\.,;:\s@\"]+(\.[^<>()[\]\\.,;:\s@\"]+)*)|(\".+\"))@((\[[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\])|(([a-zA-Z\-0-9]+\.)+[a-zA-Z]{2,}))$/;

const COOKIE_DISCLAIMER = ` For legal reasons, you need to accept the fact that this site requires 1
session cookie for all operations where you might create new content.  It is impossible to opt-out
of this cookie; however, you can still view recipes to your heart's content without using cookies.
`;

// VError: a validation error constructor
function VError(prop, msg) {
    return { prop: prop, msg: msg };
}

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

// User Object
class User {
    constructor(context = "data") {
        this.username = "";
        this.password = "";
        this.email = "";
        this.verify = "";

        this.context = context;

        const contexts = [
            "newuser", "login", "data"
        ];

        if (!contexts.includes(this.context)) {
            throw new Error(`'${this.context}' is an invalid user context`);
        }
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

    setContext(value) {
        this.context = value;
    }

    // validate: validates the user object for login or create purposes
    validate() {
        // TODO (Brian) add in validation logic

        const errors = [];

        if (this.context === "data") {
            return errors;
        }

        if (this.context === "login" || this.context === "newuser") {
            // the username must be < 40 chars
            if (this.username.length === 0) {
                errors.push(new VError("username", "You must provide a value!"));
            } else if (this.username.length > 50) {
                errors.push(new VError("username", "Must be less than 50 characters!"));
            } else if (0 <= this.username.indexOf(" ")) {
                errors.push(new VError("username", "Cannot contain a space!"));
            }
        }

        if (this.context === "newuser") {
            // email must be real
            if (this.email.length === 0) {
                errors.push(new VError("email", "You must provide a value!"));
            } else if (!email_re.test(this.email)) {
                errors.push(new VError("email", "You must provide a valid email!"));
            }
        }

        if (this.context === "login") {
            // pass && 6 < pass.len
            if (this.password.length === 0) {
                errors.push(new VError("password", "You must provide a value!"));
            } else if (this.password.length < 6) {
                errors.push(new VError("password", "Your password must be at least 6 characters!"));
            }
        }

        if (this.context === "login" || this.context === "newuser") {
            // pass && 6 < pass.len
            if (this.verify.length === 0) {
                errors.push(new VError("verify", "You must provide a value!"));
            } else if (this.password !== this.verify) {
                errors.push(new VError("verify", "The passwords must match!"));
            }
        }

        return errors;
    }

    // create: calls the backend to create a new user
    create() {
        const errors = this.validate();

        if (0 < errors.length) {
            return null;
        }

        if (this.context !== "newuser") {
            return null;
        }

        const data = {
            username: this.username,
            password: this.password,
            verify: this.verify,
            email: this.email,
        };

        return m.request({
            method: "POST",
            url: "/api/v1/user/create",
            body: data,
        });
    }

    // login: logs the user described in this object in
    login() {
        const errors = this.validate();

        if (0 < errors.length) {
            return null;
        }

        if (this.context !== "login") {
            return null;
        }

        const data = {
            username: this.username,
            password: this.password,
        };

        return m.request({
            method: "POST",
            url: "/api/v1/user/create",
            body: data,
        });
    }
}

// LoginComponent
function LoginComponent(inivialVnode) {
    const user = new User();

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
                                oninput: function (e) { user.setUsername(e.target.value); },
                                value: user.username,
                            }),
                        ]),

                        m("div", [
                            m("label", "Password"),
                            m("input[type=password]", {
                                oninput: function (e) { user.setPassword(e.target.value); },
                                value: user.password,
                            }),
                        ]),

                        m("div", [
                            m("button.button[type=button]", {
                                onclick: function() { user.login(); }
                            }, "Login"),
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
    const user = new User("newuser");

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
                                oninput: function (e) { user.setUsername(e.target.value); },
                                value: user.username,
                            }),
                        ]),

                        m("div", [
                            m("label", "Email"),
                            m("input[type=email]", {
                                oninput: function (e) { user.setEmail(e.target.value); },
                                value: user.email,
                            }),
                        ]),

                        m("div", [
                            m("label", "Password"),
                            m("input[type=password]", {
                                oninput: function (e) { user.setPassword(e.target.value); },
                                value: user.password,
                            }),
                        ]),

                        m("div", [
                            m("label", "Verify Password"),
                            m("input[type=password]", {
                                oninput: function (e) { user.setVerify(e.target.value); },
                                value: user.verify,
                            }),
                        ]),

                        m("div", [
                            m("button.button[type=submit]", {
                                onclick: function() { user.create(); }
                            }, "Save"),
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
