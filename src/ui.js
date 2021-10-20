// Brian Chrzanowski
// 2021-09-07 01:17:08
//
// There are a few interesting things you should know about the application.
//
// Basically, all of the form elements that revolve around using and setting data in a form all have
// a backing data structure / class that has all of the smarts for the form.
//
// As an example, the Recipe class has an 'errors' object inside of it, with the errors for every
// single field as they apply. When the 'InputComponent' creates itself, it reads from that 'errors'
// object, and determines if it needs to print out an error below the input, for example.

const email_re = /^(([^<>()[\]\\.,;:\s@\"]+(\.[^<>()[\]\\.,;:\s@\"]+)*)|(\".+\"))@((\[[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\])|(([a-zA-Z\-0-9]+\.)+[a-zA-Z]{2,}))$/;

const COOKIE_DISCLAIMER = `For legal reasons, you need to accept the fact that this site requires 1
session cookie for all operations where you might create new content.  It is impossible to opt-out
of this cookie; however, you can still view recipes to your heart's content without using cookies.`;

// VError: a validation error constructor
function VError(prop, msg) {
    return { prop: prop, msg: msg };
}

// H1: returns m("h1", text)
function H1(text) {
    return m("h1", text);
}

// H2: returns m("h2", text)
function H2(text) {
    return m("h2", text);
}

// H3: returns m("h3", text)
function H3(text) {
    return m("h3", text);
}

// H4: returns m("h4", text)
function H4(text) {
    return m("h4", text + "");
}

// P : paragraph wrapper
function P(text) {
    return m("p", text);
}

// DIV: returns m("div", arg)
function DIV(arg) {
    return m("div", arg);
}

// Button: returns a small button wrapper
function Button(text, fn) {
    return m("button[type=button]", { onclick: (e) => fn(e) }, text);
}

// InputComponent: a little more complicated than the others
function InputComponent(vnode) {
    let object = vnode.attrs.object;
    let prop = vnode.attrs.prop;
    let type = vnode.attrs.type;

    const types = [ "text", "number", "email" ];

    if (!type) {
        type = "text";
    }

    if (types.findIndex(x => x === type) < 0) {
        throw new Error(`${type} not found in allowed types array: ${types}`);
    }

    return {
        view: function(vnode) {
            return m("input[autocomplete=off]", {
                value: object[prop],
                oninput: (e) => {
                    if (type === "text") {
                        object[prop] = e.target.value;
                    } else if (type === "number") {
                        const strval = e.target.value.replace(/\D/g, "");
                        object[prop] = parseInt(strval);
                    } else if (type === "email") {
                        object[prop] = e.target.value.replace(/ /g, "");
                    }
                },
            });
        }
    };
}

// TextAreaComponent: a small text area wrapper thing
function TextAreaComponent(vnode) {
    let object = vnode.attrs.object;
    let prop = vnode.attrs.prop;

    let cols = vnode.attrs.cols;
    let rows = vnode.attrs.rows;
    let maxlen = vnode.attrs.maxlen;

    const coalesce = (x, def) => {
        if (x !== +x)
            return def;
        return +x;
    };

    cols = coalesce(cols, 5);
    rows = coalesce(rows, 5);
    maxlen = coalesce(maxlen, 40);

    return {
        view: function(vnode) {
            return m(`textarea`, {
                cols: cols,
                rows: rows,
                maxlength: maxlen,

                value: object[prop],
                oninput: (e) => {
                    object[prop] = e.target.value;
                },
            });
        }
    };
}

// ListComponent: a component that helps us manipulate and mangle lists
function ListComponent(vnode) {
    let list = vnode.attrs.list;
    let type = vnode.attrs.type;
    let isview = vnode.attrs.isview ?? false;

    if (list.length === 0) {
        list.push("");
    }

    if (!type) {
        type = "ul";
    }

    const add_btn = m("button[type=button]", {
        // add a new item at the end of the list
        onclick: () => list.push("")
    }, "+");

    const viewfn = function(innervnode) {
        const items = list.map((e, i, a) => {
            return m("li", a[i]);
        });

        return m(type, items);
    }

    const inputfn = function(innervnode) {
        const items = list.map((e, i, a) => {
            const is_last = i === a.length - 1;

            const content = m("input", {
                value: a[i],
                oninput: (e) => a[i] = e.target.value,
            });

            const sub_btn = m("button[type=button]", {
                onclick: () => list.splice(i, 1)
            }, "-");

            let controls = [ content ];

            if (a.length > 1) {
                controls.push(sub_btn);
            }

            return m("li", controls);
        });

        let controls = [ items, add_btn ];

        return m(type, controls);
    }

    return {
        view: isview ? viewfn : inputfn
    };
}

// MenuComponent
function MenuComponent(vnode) {
    return {
        view: function(vnode) {
            return m("nav", [
                m(m.route.Link, { href: "/" }, "Home"),
                m(m.route.Link, { href: "/recipe/new" }, "New Recipe"),
                m(m.route.Link, { href: "/search" }, "Search"),
            ]);
        }
    };
}

// Recipe: the Recipe data structure
class Recipe {
    constructor(id) {
        this.id = id;

        this.init();

        if (this.id !== undefined) {
            this.fetch();
        }
    }

    // init : initializes a blank Recipe
    init() {
        this.name = null;

        this.cook_time = null;
        this.prep_time = null;

        this.note = null;

        this.ingredients = [];
        this.steps = [];
        this.tags = [];
    }

    // fetch : fetches from the remote
    fetch() {
        return m.request({
            method: "GET",
            url: `/api/v1/recipe/${this.id}`,
        }).then((x) => {
            this.id = x.id;

            this.name = x.name;

            this.cook_time = x.cook_time;
            this.prep_time = x.prep_time;
            this.servings = x.servings;

            this.note = x.note;

            this.ingredients = x.ingredients;
            this.steps = x.steps;
            this.tags = x.tags;

            m.redraw();
        }).catch((err) => {
            console.error(err);
        });
    }

    load(obj) {
    }

    // isValid : returns true if the object is valid for upserting
    isValid() {
        // NOTE (Brian): we need to perform UI input checking
        return true;
    }

    // submit : attempts to submit the current object to the backend
    submit() {
        if (this.isValid()) {
            m.request({
                method: "POST",
                url: `/api/v1/recipe`,
                body: this
            }).then((x) => {
                console.log(x);
            }).catch((err) => {
                console.error(err);
            });
        } else {
            console.error("this is invalid!");
        }
    }

    // remove : attempts to remove the recipe from the database
    remove() {
        if (this.id !== undefined && this.id >= 0) {
            return m.request({
                method: "DELETE",
                url: `/api/v1/recipe/${this.id}`
            });
        } else {
            return Promise.reject(new Error(`cannot delete a recipe that doesn't exist!`));
        }
    }
}

// RecipeViewComponent : Handles the Reading of a Recipe
function RecipeViewComponent(vnode) {
    let id = m.route.param("id");
    let recipe = new Recipe(id);

    console.log(recipe);

    return {
        view: function(vnode) {
            let content;

            if (recipe.name === null || recipe.name === "") {
                content = m("p", "Now Loading...");
            } else {

                content = [
                    H2(recipe.name),

                    DIV([
                        H3("Preparation Time"),
                        P(recipe.prep_time),

                        H3("Cook Time"),
                        P(recipe.cook_time),

                        H3("Servings"),
                        P(recipe.servings),

                        H3("Ingredients"),
                        m(ListComponent, {
                            list: recipe.ingredients, type: "ul", isview: true
                        }),

                        H3("Steps"),
                        m(ListComponent, {
                            list: recipe.steps, type: "ol", isview: true
                        }),

                        H3("Tags"),
                        m(ListComponent, {
                            list: recipe.tags, type: "ul", isview: true
                        }),

                        H3("Notes"),
                        m("p", recipe.note),

                        Button("Edit", (e) => m.route.set(`/recipe/${recipe.id}/edit`))
                    ])
                ];
            }

            return [
                m(MenuComponent),
                content
            ];
        }
    };
}

// RecipeEditComponent : Handles Recipe CRUD Operations
function RecipeEditComponent(vnode) {
    let id = m.route.param("id");
    let recipe = new Recipe(id);

    return {
        view: function(vnode) {
            // testing just with the name
            let name_ctrl = m(InputComponent, {
                object: recipe, prop: "name",
            });

            let cook_time_ctrl = m(InputComponent, {
                object: recipe, prop: "cook_time", type: "number",
            });

            let prep_time_ctrl = m(InputComponent, {
                object: recipe, prop: "prep_time", type: "number",
            });

            let servings_ctrl = m(InputComponent, {
                object: recipe, prop: "servings", type: "number",
            });

            let notes_ctrl = m(TextAreaComponent, {
                object: recipe, prop: "note", rows: 15, cols: 100
            });

            let ingredients_ctrl = m(ListComponent, {
                list: recipe.ingredients, type: "ul",
            });

            let steps_ctrl = m(ListComponent, {
                list: recipe.steps, type: "ol",
            });

            let tags_ctrl = m(ListComponent, {
                list: recipe.tags, type: "ul",
            });

            let log_button = Button("Dump Object State", (e) => console.log(recipe));

            let submit_button = Button("Submit", (e) => recipe.submit());

            let cancel_button = Button("Cancel", (e) => m.route.set("/"));

            let delete_button = Button("Delete", (e) => {
                recipe.remove()
                    .then(() => m.route.set("/"))
                    .catch((x) => console.error(x));
            });

            return [
                m(MenuComponent),

                H3(id ? recipe.name : "New Recipe"),

                DIV([
                    H4("Name"), name_ctrl,
                    H4("Cook Time"), cook_time_ctrl,
                    H4("Prep Time"), prep_time_ctrl,
                    H4("Servings"), servings_ctrl,
                    H4("Ingredients"), ingredients_ctrl,
                    H4("Steps"), steps_ctrl,
                    H4("Tags"), tags_ctrl,
                    H4("Notes"), notes_ctrl,

                    DIV([ log_button, submit_button, cancel_button, delete_button ])
                ]),
            ];
        }
    }
};

// SearchComponent: 
function SearchComponent(vnode) {
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
function HomeComponent(vnode) {
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
function SuccessComponent(vnode) {
    const { message, next, timeout } = vnode.attrs;

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
function ErrorComponent(vnode) {
    const { message, next, timeout } = vnode.attrs;

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

        if (this.context === "newuser") {
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
            url: "/api/v1/user/login",
            body: data,
        });
    }
}

// LoginComponent
function LoginComponent(inivialVnode) {
    const user = new User("login");

    return {
        view: function(vnode) {
            return m(".login", [
                m("form", {
                    onsubmit: function (e) {
                        e.preventDefault();
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
                                onclick: function() {
                                    user.login()
                                        .then((x) => {
                                            m.route.set("/");
                                        })
                                        .catch((err) => {
                                            console.error(err);
                                        });
                                }
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
function NewUserComponent(vnode) {
    const user = new User("newuser");

    return {
        view: function(vnode) {
            return m(".newuser", [
                m("form", {
                    onsubmit: function (e) {
                        e.preventDefault();
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
                                onclick: function() {
                                    user.create()
                                        .then((x) => {
                                            m.route.set("/");
                                        })
                                        .catch((err) => {
                                            console.error(err);
                                        });
                                }
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

    "/recipe/new": RecipeEditComponent,
    "/recipe/:id": RecipeViewComponent,
    "/recipe/:id/edit": RecipeEditComponent,
};

m.route(root, "/", routes);

