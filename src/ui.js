// Brian Chrzanowski
// 2021-09-07 01:17:08

const EMAIL_REGEX = /^(([^<>()[\]\\.,;:\s@\"]+(\.[^<>()[\]\\.,;:\s@\"]+)*)|(\".+\"))@((\[[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\])|(([a-zA-Z\-0-9]+\.)+[a-zA-Z]{2,}))$/;

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

// Divider : returns a content divider
function Divider() {
    return m("div", { class: "mui-divider" });
}

// Button: returns a small button wrapper
function Button(text, fn) {
    return m("button[type=button]", {
        onclick: (e) => fn(e), class: "mui-btn mui-btn--raised"
    }, text);
}

// ButtonPrimary : makes a button with a primary color
function ButtonPrimary(text, fn) {
    return m("button[type=button]", {
        onclick: (e) => fn(e), class: "mui-btn mui-btn--raised mui-btn--primary"
    }, text);
}

// ButtonDanger : makes a button with some dangerous side effect
function ButtonDanger(text, fn) {
    return m("button[type=button]", {
        onclick: (e) => fn(e), class: "mui-btn mui-btn--raised mui-btn--danger"
    }, text);
}

// ButtonAccent : makes a button with the accent color
function ButtonAccent(text, fn) {
    return m("button[type=button]", {
        onclick: (e) => fn(e), class: "mui-btn mui-btn--raised mui-btn--accent"
    }, text);
}

// InputComponent: a little more complicated than the others
function InputComponent(vnode) {
    let object = vnode.attrs.object;
    let prop = vnode.attrs.prop;
    let label = vnode.attrs.label;
    let type = vnode.attrs.type;

    const types = [ "text", "number", "email", "password" ];

    if (!type) {
        type = "text";
    }

    if (types.findIndex(x => x === type) < 0) {
        throw new Error(`${type} not found in allowed types array: ${types}`);
    }

    return {
        view: function(vnode) {
            const contents = [];

            const input = m("input", {
                autocomplete: "nope",
                value: object[prop],
                type: type,
                oninput: (e) => {
                    if (type === "number") {
                        const strval = e.target.value.replace(/\D/g, "");
                        if (strval === "") {
                            object[prop] = null;
                        } else {
                            object[prop] = parseInt(strval);
                        }
                    } else if (type === "email") {
                        object[prop] = e.target.value.replace(/ /g, "");
                    } else {
                        object[prop] = e.target.value;
                    }
                },
            });

            contents.push(input);

            if (label) {
                contents.push(m("label", label));
            }

            return m("div", { class: "mui-textfield mui-textfield--float-label" }, contents);
        }
    };
}

// TextAreaComponent: a small text area wrapper thing
function TextAreaComponent(vnode) {
    let object = vnode.attrs.object;
    let prop = vnode.attrs.prop;
    let label = vnode.attrs.label;

    let cols = vnode.attrs.cols;
    let rows = vnode.attrs.rows;
    let maxlen = vnode.attrs.maxlen;

    const isnum = (x) => {
        return x == +x && parseInt(x) === +x;
    };

    const props = {};

    if (isnum(cols)) {
        cols = +cols;
        props["cols"] = +cols;
    }

    if (isnum(rows)) {
        rows = +rows;
        props["rows"] = +rows;
    }

    if (isnum(maxlen)) {
        maxlen = +maxlen;
        props["maxlen"] = +maxlen;
    }

    props["value"] = object[prop];
    props["oninput"] = (e) => {
        object[prop] = e.target.value;
    };

    return {
        view: function(vnode) {
            const contents = [];

            const textarea = m(`textarea`, props, object[prop]);

            contents.push(textarea);

            if (label) {
                contents.push(m("label", label));
            }

            return m("div", { class: "mui-textfield mui-textfield--float-label" }, contents);
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

    const header = H3(name);

    const add = ButtonPrimary("+", () => list.push(""));

    const viewfn = function(innervnode) {
        const items = list.map((e, i, a) => {
            return m("li", a[i]);
        });

        return m(type, items);
    }

    const inputfn = function(innervnode) {
        const items = list.map((e, i, a) => {
            const is_last = i === a.length - 1;

            /*
            const content = m("input", {
                autocomplete: "nope",
                value: a[i],
                oninput: (e) => a[i] = e.target.value,
            });
            */

            const content = m(TextAreaComponent, { object: a, prop: i, rows: 3 });

            const sub = ButtonAccent("-", () => list.splice(i, 1));

            // NOTE (Brian): With the button handlers, we always swap the item for the one before /
            // after it in the array. We ASSUME, that these buttons won't be shown on the first or
            // last items in the list.

            const up = Button("\u25B2", () => {
                [a[i - 1], a[i]] = [a[i], a[i - 1]];
            });

            const down = Button("\u25BC", () => {
                [a[i + 1], a[i]] = [a[i], a[i + 1]];
            });

            let controls = [ content ];

            if (a.length > 1) {
                controls.push(sub);

                if (0 < i) {
                    controls.push(up);
                }

                if (i < a.length - 1) {
                    controls.push(down);
                }
            }

            return m("li", controls);
        });

        let controls = [ m("div", [ header, add ]), items ];

        return m(type, controls);
    }

    return {
        view: isview ? viewfn : inputfn
    };
}

// MenuComponent : draws the menu at the top(ish) of the screen
function MenuComponent(vnode) {
    return {
        view: function(vnode) {
            return m("div", { class: "mui-appbar", style: "padding: 0 2em" }, 
                m("table", { width: "100%" },
                    m("tr", { style: "vertical-align:middle" }, [

                        m("td", { class: "mui--appbar-height" }, m(m.route.Link, {
                            href: "/", style: "color: white"
                        }, "Home")),

                        m("td", { class: "mui--appbar-height" }, m(m.route.Link, {
                            href: "/recipe/new", style: "color: white"
                        }, "New Recipe")),

                        m("td", { class: "mui--appbar-height", align: "right" }, m(m.route.Link, {
                            href: "/newuser", style: "color: white"
                        }, "New User / Login")),
                    ])
                )
            );
        }
    };
}

// Recipe: the Recipe data structure
class Recipe {
    constructor(id) {
        this.isLoading = true;

        if (id == parseInt(id)) {
            this.id = parseInt(id);
            this.fetch();
        } else {
            this.init();
            this.isLoading = false;
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

            this.isLoading = false;

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
            if (this.id === undefined) {
                return m.request({
                    method: "POST",
                    url: `/api/v1/recipe`,
                    body: this
                });
            } else {
                return m.request({
                    method: "PUT",
                    url: `/api/v1/recipe/${this.id}`,
                    body: this
                });
            }
        } else {
            return Promise.reject(new Error(`cannot submit a recipe that is invalid!`));
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

    return {
        view: function(vnode) {
            let content;

            if (recipe.isLoading) {
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
                            name: "Ingredients", list: recipe.ingredients, type: "ul", isview: true
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
            if (recipe.isLoading) {
                return  [
                    m(MenuComponent),
                    m("div", { class: "mui-container-fluid" }, [
                        P("Now Loading...")
                    ])
                ];
            }

            // testing just with the name
            let name_ctrl = m(InputComponent, {
                object: recipe, prop: "name", label: "Name"
            });

            let cook_time_ctrl = m(InputComponent, {
                object: recipe, prop: "cook_time", label: "Cook Time (HH:MM)"
            });

            let prep_time_ctrl = m(InputComponent, {
                object: recipe, prop: "prep_time", label: "Prep Time (HH:MM)"
            });

            let servings_ctrl = m(InputComponent, {
                object: recipe, prop: "servings", label: "Servings"
            });

            let notes_ctrl = m(TextAreaComponent, {
                object: recipe, prop: "note", rows: 15, cols: 100, maxlen: 256, label: "Notes"
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

            let submit_button = ButtonPrimary(recipe.id !== undefined ? "Save" : "Create",
                (e) => {
                    recipe.submit()
                        .then((x) => { m.route.set(`/recipe/${x.id}`); })
                        .catch((err) => console.error(err));
                }
            );

            let cancel_button = Button("Cancel", (e) => m.route.set("/"));

            let delete_button = ButtonDanger("Delete", (e) => {
                recipe.remove()
                    .then(() => m.route.set("/"))
                    .catch((x) => console.error(x));
            });

            let buttons = [];

            if (recipe.id === undefined) { // create
                buttons = [ log_button, submit_button, cancel_button ];
            } else { // edit
                buttons = [ log_button, submit_button, cancel_button, delete_button ];
            }

            return [
                m(MenuComponent),

                H3(id ? recipe.name : "New Recipe"),

                m("div", { class: "mui-container-fluid" }, [
                    m("div", { class: "mui-row" }, [
                        m("div", { class: "mui-col-md-12" }, name_ctrl)
                    ]),
                    m("div", { class: "mui-row" }, [
                        m("div", { class: "mui-col-md-4" }, prep_time_ctrl),
                        m("div", { class: "mui-col-md-4" }, cook_time_ctrl),
                        m("div", { class: "mui-col-md-4" }, servings_ctrl),
                    ]),
                    H4("Ingredients"), ingredients_ctrl,
                    H4("Steps"), steps_ctrl,
                    H4("Tags"), tags_ctrl,
                    notes_ctrl,

                    DIV(buttons)
                ])
            ];
        }
    }
};

// SearchComponent: handles the searching of recipes
function SearchComponent(vnode) {
    let results = [];
    let isLoading = true;

    // NOTE (Brian): we probably want a semi-smart way of handling these default values

    const query = {
        text: "",
        pageSize: 20,
        pageNumber: 0,
    };

    // doSearch : this function sets up the request and returns a Promise for it
    const doSearch = () => {
        // TODO (Brian): we need to have optional parameter things in here
        // Like, you shouldn't always have to send pageSize, or pageNumber
        const qString = `q=${query.text}&siz=${query.pageSize}&num=${query.pageNumber}`;

        isLoading = true;

        return m.request({
            method: "GET",
            url: `/api/v1/recipe/list?${qString}`,
        })
    };

    // enterHandler : does the searching, and hooks back up the search results
    const enterHandler = () => {
        doSearch().then((x) => {
            results = x;
            console.log(results);
            m.redraw();
        }).catch((err) => {
            console.error(err);
        }).finally(() => {
            isLoading = false;
        });
    };

    // displayResults : returns the "body" of the component, regardless of what condition the
    // results are in
    const displayResults = () => {
        if (isLoading) {
            // TODO (Brian): return the loading widget
            return m("p", "Loading...");
        }

        if (results.length === 0) {
            // TODO (Brian): display the no results thing
            return m("p", "No results!");
        } else {
            return m("table", { class: "mui-table" }, [
                m("thead", [
                    m("tr", [
                        m("th", "Name"),
                        m("th", "Prep Time"),
                        m("th", "Cook Time"),
                        m("th", "Servings"),
                    ])
                ]),
                m("tbody",
                    results.map((e) => {
                        return m("tr", [
                            m("td", m(m.route.Link, {
                                href: `/recipe/${e.id}`
                            }, e.name)),
                            m("td", e.prep_time),
                            m("td", e.cook_time),
                            m("td", e.servings),
                        ]);
                    })
                )
            ])
        }
    };

    enterHandler(); // like we performed a blank search at the start of the page

    return {
        view: function(vnode) {
            return [
                m("div", { class: "mui-textfield mui-textfield--float-label" }, [
                    m("input", {
                        autocomplete: "nope",
                        value: query.text,
                        oninput: (e) => {
                            query.text = e.target.value;
                        },
                        onkeyup: (e) => {
                            if (e.key === "Enter" || e.keycode === 113) {
                                enterHandler();
                            }
                        }
                    }),
                    m("label", "Search for a Recipe...")
                ]),
                displayResults()
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
                m(SearchComponent)
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

function UserValidate(user) {
    // TODO (Brian) add in validation logic

    const errors = [];

    if (user.context === "data") {
        return errors;
    }

    if (user.context === "login" || user.context === "newuser") {
        // the username must be < 40 chars
        if (user.username.length === 0) {
            errors.push(new VError("username", "You must provide a value!"));
        } else if (user.username.length > 50) {
            errors.push(new VError("username", "Must be less than 50 characters!"));
        } else if (0 <= user.username.indexOf(" ")) {
            errors.push(new VError("username", "Cannot contain a space!"));
        }
    }

    if (user.context === "newuser") {
        // email must be real
        if (user.email.length === 0) {
            errors.push(new VError("email", "You must provide a value!"));
        } else if (!EMAIL_REGEX.test(user.email)) {
            errors.push(new VError("email", "You must provide a valid email!"));
        }
    }

    if (user.context === "login") {
        // pass && 6 < pass.len
        if (user.password.length === 0) {
            errors.push(new VError("password", "You must provide a value!"));
        } else if (user.password.length < 6) {
            errors.push(new VError("password", "Your password must be at least 6 characters!"));
        }
    }

    if (user.context === "newuser") {
        // pass && 6 < pass.len
        if (user.verify.length === 0) {
            errors.push(new VError("verify", "You must provide a value!"));
        } else if (user.password !== user.verify) {
            errors.push(new VError("verify", "The passwords must match!"));
        }
    }

    return errors;
}

// LoginComponent
function LoginComponent(inivialVnode) {
    const user = {
        context: "login"
    };

    return {
        view: function(vnode) {
            let email = m(InputComponent, {
                object: user, prop: "email", label: "Email", type: "email",
            });

            let password = m(InputComponent, {
                object: user, prop: "password", label: "Password", type: "password",
            });

            let buttons = [
                ButtonPrimary("Login", (e) => {
                    console.log("LOG ME IN!!");
                }),
                Button("Cancel", (e) => m.route.set("/"))
            ];

            return [
                m(MenuComponent),

                m("div", { class: "mui-container-fluid" }, [
                    m("div", { class: "mui-row" }, [ m("div", { class: "mui-col-md-12" }, H2("Login"),) ]),
                    m("div", { class: "mui-row" }, [ m("div", { class: "mui-col-md-12" }, email) ]),
                    m("div", { class: "mui-row" }, [ m("div", { class: "mui-col-md-12" }, password) ]),
                ]),

                DIV(buttons),

                P(COOKIE_DISCLAIMER),
            ];
        },
    };
}

// New User Component
function NewUserComponent(vnode) {
    const user = {
        context: "newuser"
    };

    return {
        view: function(vnode) {
            let username = m(InputComponent, {
                object: user, prop: "username", label: "Username",
            });

            let email = m(InputComponent, {
                object: user, prop: "email", label: "Email", type: "email",
            });

            let password = m(InputComponent, {
                object: user, prop: "password", label: "Password", type: "password",
            });

            let verify = m(InputComponent, {
                object: user, prop: "verify", label: "Verify Password", type: "password",
            });

            let buttons = [
                ButtonPrimary("Create", (e) => {
                    console.log("CREATE MY USER!!");
                }),
                Button("Cancel", (e) => m.route.set("/"))
            ];

            return [
                m(MenuComponent),

                m("div", { class: "mui-container-fluid" }, [
                    m("div", { class: "mui-row" }, [ m("div", { class: "mui-col-md-12" }, H2("New User"),) ]),
                    m("div", { class: "mui-row" }, [ m("div", { class: "mui-col-md-12" }, username) ]),
                    m("div", { class: "mui-row" }, [ m("div", { class: "mui-col-md-12" }, email) ]),
                    m("div", { class: "mui-row" }, [ m("div", { class: "mui-col-md-12" }, password) ]),
                    m("div", { class: "mui-row" }, [ m("div", { class: "mui-col-md-12" }, verify) ]),
                ]),

                DIV(buttons),

                P(COOKIE_DISCLAIMER),
            ];
        },
    };
}

// hook it all up
var root = document.body;

const routes = {
    "/": HomeComponent,
    "/newuser": NewUserComponent,
    "/login": LoginComponent,

    "/recipe/new": RecipeEditComponent,
    "/recipe/:id": RecipeViewComponent,
    "/recipe/:id/edit": RecipeEditComponent,
};

m.route(root, "/", routes);

