
Create a config.h and a config.c file from a yaml definition.

It requires jansson for handling of json configuration,
and glib just in general.

It also generates a markdown table of the configuration, like this:

| Name | Short arg | Long arg | Env | Json conf | Description |
| ------ | --------- | -------- | --- | --------- | ----------- |
| main.first | f | first |  | main.first | This is a variable |
| main.second | s | second | SECOND_VAR | main.second | |
| main.third | t | third |  | main.third | |
| main.double_param | d | test-double |  | main.double | |
| main.deep.param |  |  |  | main.deep.param | |
| main.deep.enumtest | e | enum-test | enum-test | main.deep.param_enum | |
| main.deep.params |  |  |  | main.deep.params | |
| other |  |  |  | other | |
