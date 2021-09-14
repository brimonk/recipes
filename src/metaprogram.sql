select
    m.name table_name
    , ti.cid as ordinal_pos
    , ti.name as colname
    , ti.type as coltype
    , not ti.[notnull] as 'isnull'
    , ti.pk as ispk
from sqlite_master as m
    , pragma_table_info(m.name) as ti
where m.type = 'table';

