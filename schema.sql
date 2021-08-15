-- Brian Chrzanowski
-- 2021-06-01 22:48:34

create table if not exists user
(
      id         text default (uuid())
    , created_ts text default (strftime('%Y-%m-%dT%H:%M:%S', 'now'))
    , updated_ts text default null

    , username   text not null
    , password   text not null
    , email      text not null

    , is_verified int default (0)
);

create unique index if not exists idx_user_id on user(id);
create unique index if not exists idx_user_name on user(username);
create unique index if not exists idx_user_email on user(email);

-- user_session: stores sessions here, they expire in a week
create table if not exists user_session
(
      user_id      text not null
    , session_id   blob not null
    , expires_ts   text default (strftime('%Y-%m-%dT%H:%M:%S', 'now', '+7 days'))

    , foreign key (user_id) references user(id)
);

create unique index if not exists idx_usersession_id on user_session(user_id);

create table if not exists recipe
(
      id         text default (uuid())
    , created_ts text default (strftime('%Y-%m-%dT%H:%M:%S', 'now'))
    , updated_ts text default null

    , name      text not null

    , prep_time integer not null
    , cook_time integer not null

    , note      text null
);

create unique index if not exists idx_recipe_id on recipe(id);

create view if not exists v_list_recipe
as
select created_ts as ts, name from recipe;

create table if not exists ingredient
(
      id         text default (uuid())
    , created_ts text default (strftime('%Y-%m-%dT%H:%M:%S', 'now'))
    , updated_ts text default null

    , recipe_id  text not null

    , desc       text not null

    , sort       integer not null

    , foreign key (recipe_id) references recipe(id)
);

create unique index if not exists idx_ingredient_id on ingredient(id);

create table if not exists step
(
      id         text default (uuid())
    , created_ts text default (strftime('%Y-%m-%dT%H:%M:%S', 'now'))
    , updated_ts text default null

    , recipe_id  text not null

    , text       text not null
    , sort       integer not null

    , foreign key (recipe_id) references recipe(id)
);

create unique index if not exists idx_step_id on step(id);

create table if not exists tag
(
      id         text default (uuid())
    , created_ts text default (strftime('%Y-%m-%dT%H:%M:%S', 'now'))
    , updated_ts text default null
    , recipe_id  text not null

    , text not null

    , foreign key (recipe_id) references recipe(id)
);

create unique index if not exists idx_tag_id on tag(id);

