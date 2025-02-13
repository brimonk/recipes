// Brian Chrzanowski
// 2021-09-07 01:17:08

const EMAIL_REGEX = /^(([^<>()[\]\\.,;:\s@\"]+(\.[^<>()[\]\\.,;:\s@\"]+)*)|(\".+\"))@((\[[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\])|(([a-zA-Z\-0-9]+\.)+[a-zA-Z]{2,}))$/;

let staticData = null;

let user = null;

function RefreshUserObject() {
    m.request({
        method: "GET",
        url: `/api/v1/whoami`
    }).then((x) => {
        user = x;
    }).catch((err) => {
        // console.warn("We tried to fetch the user record, but we haven't been authenticated yet.")
    });
}

// InvalidateUserObject : just sets the 'user' object to be null
function InvalidateUserObject() {
    user = null;
}

// FetchStaticData : setup the cookie disclaimer
function FetchStaticData() {
    if (!staticData) {
        m.request({
            method: "GET",
            url: `/api/v1/static`,
        }).then((x) => {
            staticData = x;
        }).catch((err) => {
            console.error(err);
        });
    }
}

// StaticDataOrBlank : returns the state data, or empty string
function StaticDataOrBlank(x) {
    return x ?? "";
}

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

// Row : returns a row
function Row(arg) {
    return m("div", { class: "mui-row" }, arg);
}

// Col : returns a col
function Col(arg) {
    throw new Error("not implemented");
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
    let maxlen = vnode.attrs.maxlen;

    // NOTE (Brian): at one point, the InputComponent supported binding values back to a number
    // (integer / float) on the referenced object, but we kinda dropped that when everything
    // effectively became a string.

    const types = [ "text", "email", "password" ];

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
                    let value = e.target.value;

                    if (maxlen < (value?.length ?? 0)) {
                        value = value.substr(0, maxlen);
                    }

                    if (type === "email") {
                        value = value.replace(/ /g, "");
                    }

                    object[prop] = value;
                },
            });

            const characters = m(`div`, `${(object[prop]?.length ?? 0)} / ${maxlen}`);

            contents.push(input);
            contents.push(characters);

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

    const inputlen = (x) => x?.length ?? 0;

    props["value"] = object[prop];

    props["len"] = inputlen(props["value"]);

    props["oninput"] = (e) => {
        let value = e.target.value;
        let len = inputlen(value);

        const maxlen = props["maxlen"];

        if (maxlen < len) {
            value = value.substr(0, maxlen);
            len = inputlen(value);
        }

        object[prop] = value;
        props["len"] = len;
    };

    return {
        view: (vnode) => {
            const contents = [];

            const textarea = m(`textarea`, props, object[prop]);
            const characters = m(`div`, `${props["len"]} / ${props["maxlen"]}`);

            contents.push(textarea);
            contents.push(characters);

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
    let name = vnode.attrs.name;

    if (list.length === 0) {
        list.push("");
    }

    if (!type) {
        type = "ul";
    }

    const add = ButtonPrimary(`Add ${name}`, () => list.push(""));

    const viewfn = function(innervnode) {
        const items = list.map((e, i, a) => {
            return m("li", a[i]);
        });

        return m("div", [
            m("h3", `${name}s`),
            m(type, items)
        ]);
    }

    const inputfn = function(innervnode) {
        const items = list.map((e, i, a) => {
            const is_last = i === a.length - 1;

            const container = [];

            const textarea = m("div", { class: "mui-col-md-10 mui-col-xs-12" }, m(TextAreaComponent, {
                object: a, prop: i, rows: 3, maxlen: 128, label: `${name} ${i + 1}`
            }));

            const buttons = [];

            const sub = m("button[type=button]", {
                class: "mui-btn mui-btn--raised mui-btn--accent",
                onclick: () => list.splice(i, 1)
            }, "-");

            // NOTE (Brian): With the button handlers, we always swap the item for the one before /
            // after it in the array. We ASSUME, that these buttons won't be shown on the first or
            // last items in the list.

            const up = Button("\u25B2", () => [a[i - 1], a[i]] = [a[i], a[i - 1]]);

            const down = Button("\u25BC", () => [a[i + 1], a[i]] = [a[i], a[i + 1]]);

            container.push(textarea);

            if (a.length > 1) {
                buttons.push(sub);

                if (0 < i) {
                    buttons.push(up);
                }

                if (i < a.length - 1) {
                    buttons.push(down);
                }
            }

            container.push(m("div", { class: "mui-col-md-2 mui-col-xs-6" }, buttons));

            return m("div", { class: "mui-row" }, container);
        });

        return m("div", [ items, add ]);
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

        if (id == id) {
            this.id = id;
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
        if (this.id !== undefined) {
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
                let notesComponent;

                if (recipe.note) {
                    notesComponent = DIV([
                        H3("Notes"),
                        m("p", recipe.note),
                    ]);
                } else {
                    notesComponent = null;
                }

                content = [
                    H2(recipe.name),

                    DIV([
                        H3("Preparation Time"),
                        P(recipe.prep_time),

                        H3("Cook Time"),
                        P(recipe.cook_time),

                        H3("Servings"),
                        P(recipe.servings),

                        m(ListComponent, {
                            name: "Ingredient", list: recipe.ingredients, type: "ul", isview: true
                        }),

                        m(ListComponent, {
                            name: "Step", list: recipe.steps, type: "ol", isview: true
                        }),

                        m(ListComponent, {
                            name: "Tag", list: recipe.tags, type: "ul", isview: true
                        }),

                        notesComponent,

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

    m.request({
        method: "GET",
        url: `/api/v1/tags`,
    }).then((x) => {
        console.log(x);
    }).catch((err) => {
        console.error(err);
    });

    return {
        view: function(vnode) {
            // TEMPORARY

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
                object: recipe, prop: "name", label: "Name", maxlen: 128
            });

            let cook_time_ctrl = m(InputComponent, {
                object: recipe, prop: "cook_time", label: "Cook Time (HH:MM)", maxlen: 128
            });

            let prep_time_ctrl = m(InputComponent, {
                object: recipe, prop: "prep_time", label: "Prep Time (HH:MM)", maxlen: 128
            });

            let servings_ctrl = m(InputComponent, {
                object: recipe, prop: "servings", label: "Servings", maxlen: 128
            });

            let notes_ctrl = m(TextAreaComponent, {
                object: recipe, prop: "note", rows: 15, cols: 100, maxlen: 256, label: "Notes"
            });

            let ingredients_ctrl = m(ListComponent, {
                list: recipe.ingredients, type: "ul", name: "Ingredient"
            });

            let steps_ctrl = m(ListComponent, {
                list: recipe.steps, type: "ol", name: "Step"
            });

            let tags_ctrl = m(ListComponent, {
                list: recipe.tags, type: "ul", name: "Tag"
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

                m("div", { class: "mui-container-fluid" }, [
                    H2(!id ? "New Recipe" : null),
                    m("div", { class: "mui-row" }, [
                        m("div", { class: "mui-col-md-12" }, name_ctrl)
                    ]),
                    m("div", { class: "mui-row" }, [
                        m("div", { class: "mui-col-md-4" }, prep_time_ctrl),
                        m("div", { class: "mui-col-md-4" }, cook_time_ctrl),
                        m("div", { class: "mui-col-md-4" }, servings_ctrl),
                    ]),
                    ingredients_ctrl,
                    steps_ctrl,
                    tags_ctrl,
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

    // TODO (Brian):
    //
    // It'd be great if the table had the "No results!" message in the middle of the table, _with_
    // the columns present.
    //
    // We should maybe only give parameters when they're filled with data and such.
    //
    // The "First-Last of Total" in the pagination bar seems a little shitty.

    const query = {
        text: "",
        pageSize: 20,
        pageNumber: 0,
        total: 0
    };

    const elementQuantities = [ 5, 10, 20, 50, 100 ];

    // doSearch : this function sets up the request and returns a Promise for it
    const doSearch = () => {
        // TODO (Brian): we need to have optional parameter things in here
        // Like, you shouldn't always have to send pageSize, or pageNumber
        const qString = `q=${query.text}&siz=${query.pageSize}&num=${query.pageNumber}`;

        isLoading = true;

        return m.request({
            method: "GET",
            url: `/api/v1/recipe/list?${qString}`,
        });
    };

    // enterHandler : does the searching, and hooks back up the search results
    const enterHandler = () => {
        doSearch().then((x) => {
            query.total = x.total;
            results = x.results;
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

        let startidx, endidx;

        startidx = query.pageSize * query.pageNumber + 1;
        endidx = (query.pageSize * (query.pageNumber + 1));

        if (query.total < endidx) {
            endidx = query.total;
        }

        if (endidx === 0) {
            startidx = 0;
        }

        const table = m("table", { class: "mui-table" }, [
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
        ]);

        return m("div", [
            (results.length === 0 ? m("p", "No results!") : table),
            m("div", { class: "mui-row", style: "background: lightgray; padding: 8px" }, [

                // NOTE (Brian): this monstrosity is to show the page size selector
                m("div", { class: "mui-col-md-4 mui-dropdown" }, [
                    m("button", { class: "mui-btn mui-btn--raised", "data-mui-toggle": "dropdown" }, [
                        `Items Per Page: ${query.pageSize}`, m("span", { class: "mui-caret" })
                    ]),
                    m("ul", { class: "mui-dropdown__menu" },
                        elementQuantities.map((x) => m("li", m("a", {
                            href: "#",
                            onclick: (e) => {
                                query.pageSize = x;
                                enterHandler();
                            }
                        }, x)))
                    )

                ]),

                // NOTE (Brian): Now we can display information like top-bottom of total
                // and control arrows
                m("p", { class: "mui-col-md-1" },
                    `${startidx}-${endidx} of ${query.total}`),
                m("button", {
                    class: "mui-col-md-1 mui-btn", onclick: (e) => {
                        if (query.pageNumber > 0) {
                            query.pageNumber--;
                            enterHandler();
                        }
                    }
                }, "\u2039"),
                m("button", {
                    class: "mui-col-md-1 mui-btn", onclick: (e) => {
                        if (query.pageNumber < Math.floor(query.total / query.pageSize)) {
                            query.pageNumber++;
                            enterHandler();
                        }
                    }
                }, "\u203a")
            ])
        ]);
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
            return m("div", [
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
        }).then((x) => {
            RefreshUserObject();
        }).catch((err) => {
            InvalidateUserObject();
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
        }).then((x) => {
            RefreshUserObject();
        }).catch((err) => {
            InvalidateUserObject();
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

    FetchStaticData();

    return {
        view: function(vnode) {
            let email = m(InputComponent, {
                object: user, prop: "email", label: "Email", type: "email", maxlen: 128
            });

            let password = m(InputComponent, {
                object: user, prop: "password", label: "Password", type: "password", maxlen: 128
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

                P(StaticDataOrBlank(staticData?.cookieDisclaimer)),
            ];
        },
    };
}

// New User Component
function NewUserComponent(vnode) {
    const user = {
        context: "newuser"
    };

    FetchStaticData();

    return {
        view: function(vnode) {
            let username = m(InputComponent, {
                object: user, prop: "username", label: "Username", maxlen: 128
            });

            let email = m(InputComponent, {
                object: user, prop: "email", label: "Email", type: "email", maxlen: 128
            });

            let password = m(InputComponent, {
                object: user, prop: "password", label: "Password", type: "password", maxlen: 128
            });

            let verify = m(InputComponent, {
                object: user, prop: "verify", label: "Verify Password", type: "password", maxlen: 128
            });

            let buttons = [
                ButtonPrimary("Create", (e) => {
                    m.request({
                        method: "POST",
                        url: "/api/v1/newuser",
                        body: user,
                    }).then((x) => {
                        RefreshUserObject();
                        m.route.set("/");
                        console.log(x);
                    }).catch((err) => {
                        InvalidateUserObject();
                    });
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

                P(StaticDataOrBlank(staticData?.cookieDisclaimer)),
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
