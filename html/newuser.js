// Brian Chrzanowski
// 2021-08-13 02:28:39

const email_re = /^(([^<>()[\]\\.,;:\s@\"]+(\.[^<>()[\]\\.,;:\s@\"]+)*)|(\".+\"))@((\[[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\.[0-9]{1,3}\])|(([a-zA-Z\-0-9]+\.)+[a-zA-Z]{2,}))$/;

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
    const email = element("email");
    const password = element("password");
    const verify_password = element("verify-password");

    let is_valid = true;

    const focus_if_needed = (elem) => {
        if (!is_valid) {
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

    { // email
        const error_handle = element("email-errors");

        clear_node(error_handle);

        const value = email.value;

        if (value.length === 0) {
            const msg = "You must provide a value!";
            error_handle.appendChild(error_message(msg));
            focus_if_needed(email);
        } else if (!email_re.test(value)) {
            const msg = "You must provide a valid email!";
            error_handle.appendChild(error_message(msg));
            focus_if_needed(email);
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

    { // verify_password
        const error_handle = element("verify-password-errors");

        clear_node(error_handle);

        const pval = password.value;
        const vval = verify_password.value;

        if (vval.length === 0) {
            const msg = "You must provide a value!";
            error_handle.appendChild(error_message(msg));
            focus_if_needed(verify_password);
        } else if (pval !== vval) {
            const msg = "The passwords must match!";
            error_handle.appendChild(error_message(msg));
            focus_if_needed(verify_password);
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

