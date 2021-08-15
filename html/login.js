// Brian Chrzanowski
// 2021-08-13 02:28:39

// element: terse passthrough for getElementById
function element(id) {
    return document.getElementById(id);
}

// create: terse passthrough for createElement
function create(tag) {
    return document.createElement(tag);
}

// submit_handler: handles the submit button
function submit_handler() {
    const form = element("form");

    is_valid = validate();
    if (is_valid) {
        form.submit();
    }
}

// validate: validates the entire form
function validate() {
    const username = element("username");
    const password = element("password");

    // NOTE (Brian) this must mirror exactly the rules in the backend, otherwise we'll be sending
    // mixed signals to the user.

    let is_valid = true;

    const focus_if_needed = (elem) => {
        if (is_valid) {
            elem.focus();
            is_valid = false;
        }
    };

    { // username
        const error_handle = element("username-errors");

        clear_node(error_handle);

        const value = username.value;

        if (value.length === 0) {
            const msg = "You must provide a value!";
            error_handle.appendChild(error_message(msg));
            focus_if_needed(username);
        } else if (value.length > 50) {
            const msg = "Usernames have a maximum length of 50 characters.";
            error_handle.appendChild(error_message(msg));
            focus_if_needed(username);
        } else if (0 <= value.indexOf(" ")) {
            const msg = "Usernames cannot have spaces!";
            error_handle.appendChild(error_message(msg));
            focus_if_needed(username);
        }
    }

    { // password
        const error_handle = element("password-errors");

        clear_node(error_handle);

        const value = password.value;

        if (value.length === 0) {
            const msg = "You must provide a value!";
            error_handle.appendChild(error_message(msg));
            focus_if_needed(password);
        } else if (value.length < 6) {
            const msg = "Your password must be at least 6 characters!";
            error_handle.appendChild(error_message(msg));
            focus_if_needed(password);
        }
    }

    return is_valid;
}

// error_message: returns a document element with an error message
function error_message(msg) {
    const node = create("p");

    node.setAttribute("class", "text-red-400");
    node.innerText = msg;

    return node;
}

// clear_node: removes all of the children from this node
function clear_node(element) {
    while (element.firstChild) {
        element.removeChild(element.firstChild);
    }
}

