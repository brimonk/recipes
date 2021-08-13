// Brian Chrzanowski
// 2021-08-12 14:26:28

// List: helper class / functions to maintaining a list of elements
function List(id, name) {
    this.id = id;
    this.name = name;

    this.hook = document.getElementById(this.id);
    if (!this.hook) {
        throw new Error(`couldn't find element '${this.id}`);
    }

    this.count = 0;

    this.class_wrapper = "flex space-x-4 space-y-4";

    this.append();
}

// List.append: appends a new item into the list
List.prototype.append = function() {
    const element = this.get(this.count++);
    const hook = document.getElementById(this.id);
    hook.appendChild(element);
};

// List.get: gets a new list item (html / dom element)
List.prototype.get = function(idx) {
    if (idx < 0) {
        idx = 0;
    }

    const template = document.createElement("div");

    const array_prefix = `${this.name}[${idx}]`;

    template.setAttribute("id", `${array_prefix}_handle`);
    template.setAttribute("class", this.class_wrapper);

    const label = make_label((idx + 1).toString() + ".");
    const textbox = make_textbox(`${array_prefix}`);
    const xbutton = make_xbutton(`${array_prefix}_xbutton`);

    xbutton.addEventListener("click", () => {
        template.parentElement.removeChild(template);
        this.renumber(idx);
        this.count--;
    });

    template.appendChild(label);
    template.appendChild(textbox);
    template.appendChild(xbutton);

    return template;
};

// List.renumber: renumbers all of the elements in the list
List.prototype.renumber = function() {
    let j = 0;

    for (let i = 0; i < this.count; i++) {
        const item = element(`${this.name}[${i}]_handle`);
        if (item) {
            console.log(`renumbering ${i} to ${j}`);

            item.setAttribute("id", `${this.name}[${j}]_handle`);
            item.setAttribute("name", `${this.name}[${j}]_handle`);
            item.firstElementChild.innerText = `${j + 1}.`;

            // NOTE (Brian) this is kinda wonky
            const inputbox = item.childNodes[1];

            inputbox.setAttribute("id", `${this.name}[${j}]`);
            inputbox.setAttribute("name", `${this.name}[${j}]`);

            j++;
        }
    }
};

let state = {
    ingredients: new List("ingredients-hook", "ingredients"),
    steps: new List("steps-hook", "steps")
};

// element: terse passthrough for getElementById
function element(id) {
    return document.getElementById(id);
}

// create: terse passthrough for createElement
function create(tag) {
    return document.createElement(tag);
}

function step_add() {
    throw new Error("not implemented!");
}

function tag_add() {
    throw new Error("not implemented!");
}

// hookups: add button hookups
function hookups() {
    document.getElementById("ingredients_add_button").addEventListener("click", () => {
        state.ingredients.append();
    });

    document.getElementById("steps_add_button").addEventListener("click", () => {
        state.steps.append();
    });
}

// make_label: makes a label with the right styling
function make_label(text) {
    const label = create("label");

    label.setAttribute("class", "text-gray-700 text-lg space-y-28");
    label.textContent = text;

    return label;
}

// make_textbox: makes a textbox with the given id
function make_textbox(id) {
    const textbox = create("input");

    textbox.setAttribute("class", "appearance-none block w-full bg-gray-200 text-gray-700 border border-gray-200 rounded py-3 px-4 leading-tight focus:outline-none focus:bg-white focus:border-gray-500");
    textbox.setAttribute("type", "text");
    textbox.setAttribute("autocomplete", "off");

    textbox.setAttribute("id", id);
    textbox.setAttribute("name", id);

    return textbox;
}

// make_xbutton: makes an "X" button with the given id
function make_xbutton(id) {
    const button = create("input");

    button.setAttribute("class", "bg-red-300 hover:bg-red-400 text-white font-bold py-2 px-4 rounded");
    button.setAttribute("type", "button");
    button.setAttribute("value", "X");

    // TODO (Brian) setup a button handler for this thing here!

    return button;
}

// hook up the button handler
hookups();

