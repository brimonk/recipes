// Brian Chrzanowski
// 2021-08-12 14:26:28

// TODO (Brian)
// 1. add button handlers list items

let totals = {
    ingredients: 0,
    steps: 0,
    tags: 0
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
    document.getElementById("ingredient_add_button").addEventListener("click", () => {
        console.log("ingredient add button clicked!");
        add_ingredient();
    });
}

// add_ingredient: adds a new ingredient item
function add_ingredient() {
    const element = get_ingredient(totals.ingredients++);
    const hook = document.getElementById("ingredient-hook");

    hook.appendChild(element);
}

// get_ingredient: gets the ingredient template, with the array indexed
function get_ingredient(i) {
    if (i < 0) {
        i = 0;
    }

    const template = document.createElement("div");

    template.setAttribute("id", `ingredients[${i}]_handle`);
    template.setAttribute("class", "flex space-x-4 space-y-4");

    // label with index-ish
    // input with name with index
    // input button with handler and id

    const label = make_label(i.toString() + ".");
    const textbox = make_textbox(`ingredients[${i}]`);
    const remove = make_xbutton(`ingredients[${i}]_xbutton`);

    remove.addEventListener("click", () => {
        template.parentElement.removeChild(template);
        renumber_ingredients();
    });

    template.appendChild(label);
    template.appendChild(textbox);
    template.appendChild(remove);

    return template;
}

// renumber_ingredients: renumbers the ingredients list
function renumber_ingredients() {
    const handle = element("ingredient-hook");
    console.log("start");
    let j = 0;
    for (let i = 0; i < totals.ingredients; i++) {
        const item = element(`ingredients[${i}]_handle`);
        if (item) {
            console.log(`renaming ${i} to ${j}`);
            item.setAttribute("id", `ingredients[${j}]_handle`);
            item.firstElementChild.innerText = `${j++}.`;
        }
    }

    totals.ingredients--;
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

add_ingredient();

// step_add();
// tag_add();

