-- Brian Chrzanowski
-- 2023-01-10 13:07:31
--
-- Recipes Schema

-- users: table that stores all of our users
create table if not exists users (
    id             text not null default (uuid())
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
    id             text not null default (uuid())
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
    , sorting      integer not null default (0)
    , text         text not null
    , foreign key (parent_id) references recipes(id)
);

-- steps: child table to store steps
create table if not exists steps (
    parent_id      text not null
    , sorting      integer not null default (0)
    , text         text not null
    , foreign key (parent_id) references recipes(id)
);

-- tags: child table to store tags
create table if not exists tags (
    parent_id      text not null
    , sorting      integer not null default (0)
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

-- ui_recipes: a view for the recipes list page
create view if not exists ui_recipes
    (id, name, prep_time, cook_time, servings) as
select
    id
    , name
    , prep_time
    , cook_time
    , servings
from recipes;

-- v_recipe_search: the view for the recipe search screen
create view if not exists v_recipes
    (id, search_text) as
select
    r.id as id, r.search_text || '|' || i.search_text || '|' || s.search_text || '|' || t.search_text as search_text
from (select id, name || '|' || prep_time || '|' || cook_time || '|' || servings as search_text from recipes where delete_ts is null) as r
join (
    select distinct parent_id, group_concat(text, '|') over (partition by parent_id) as search_text from ingredients
) i on r.id = i.parent_id
join (
    select distinct parent_id, group_concat(text, '|') over (partition by parent_id) as search_text from steps
) s on r.id = s.parent_id
join (
    select distinct parent_id, group_concat(text, '|') over (partition by parent_id) as search_text from tags
) t on r.id = t.parent_id;
