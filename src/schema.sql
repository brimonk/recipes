-- Brian Chrzanowski
-- 2023-01-10 13:07:31
--
-- Recipes Schema

-- users: table that stores all of our users
create table if not exists users (
    id             text not null
    , create_ts    text not null default (strftime('%Y%m%d-%H%M%f', 'now'))
    , update_ts    text null
    , delete_ts    text null
    , username     text not null
    , email        text not null
    , password     text not null
    , salt         text not null
    , secret       text null -- ?
);

-- recipes: table to store our recipes
create table if not exists recipes (
    id             text not null
    , create_ts    text not null default (strftime('%Y%m%d-%H%M%f', 'now'))
    , update_ts    text null
    , delete_ts    text null
    , name         text not null
    , prep_time    text null
    , cook_time    text null
    , servings     text null
    , notes        text null
);

-- ingredients: child table to store ingredients
create table if not exists ingredients (
    parent_id      text not null
    , text         text not null
    , foreign key (parent_id) references recipes(id)
);

-- steps: child table to store steps
create table if not exists steps (
    parent_id      text not null
    , text         text not null
    , foreign key (parent_id) references recipes(id)
);

-- tags: child table to store tags
create table if not exists tags (
    parent_id      text not null
    , text         text not null
    , foreign key (parent_id) references recipes(id)
);

-- images: child table to hold images of recipes
create table if not exists images (
    user_id        text null
    , recipe_id    text null
    , ordering     integer not null default 0
    , data         blob not null
);
