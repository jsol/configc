package main

import (
	"bytes"
	"embed"
	"fmt"
	"strings"
	"text/template"
)

type SetEnv struct {
	Name string
	Env  string
}

type SetOpt struct {
	Name      string
	Long      string
	Short     string
	Type      string
	Param     string
	Desc      string
	EntryType string
	Default   string
}

type SetDefault struct {
	Name  string
	Value string
}

type Definition struct {
	Name        string
	Type        string
	Description string
	Variables   []Definition
}

type JsonParameter struct {
	Name     string
	FullName string
	Type     string
}

type JsonObject struct {
	Name           string
	Param          string
	Parent         string
	JsonParameters []JsonParameter
}

type EnumOption struct {
	EnumName string
	NiceName string
}

type Enum struct {
	Name     string
	FlatRef  string
	EnumName string
	Options  []EnumOption
}

type Output struct {
	Definitions     []Definition
	SetEnv          []SetEnv
	SetOpt          []SetOpt
	SetDefault      []SetDefault
	CheckAndSet     []string
	ValidateOptions []string
	Clear           []string
	JsonObjects     []JsonObject
	JsonParameters  []string
	OutputFormat    string
	Enums           []Enum
}

//go:embed templates/*
var templateFiles embed.FS

func structRef(p Parameter) string {
	return "cfg->" + p.Name
}

func getDefinition(def *Tree, list []Definition) []Definition {
	var out Definition

	out.Name = def.Name

	for _, v := range def.Leafs {
		if v.Def != nil {
			var ctype string
			switch v.Def.Type {
			case "string":
				ctype = "gchar *"
			case "int":
				ctype = "gint64 "
			case "size":
				ctype = "gint64 "
			case "double":
				ctype = "gdouble "
			case "boolean":
				ctype = "gboolean "
			case "enum":
				enum_name := "config_" + v.Def.FlatRef
				ctype = "enum " + enum_name + " "
			}
			out.Variables = append(out.Variables, Definition{Name: v.Name, Type: ctype, Description: v.Def.Desc})
		} else {
			out.Variables = append(out.Variables, Definition{Name: v.Name, Type: fmt.Sprintf("struct %s ", v.Name)})
			list = getDefinition(v, list)
		}
	}
	list = append(list, out)

	return list
}

func getJson(def *Tree, list *[]JsonObject, parent string) {
	var out JsonObject

	fmt.Println("Name of json node: " + def.Name)
	fmt.Println("Count of leaves: ", len(def.Leafs))
	out.Name = def.Name
	out.Parent = parent
	if parent != "" {
		out.Param = parent + "_" + def.Name
	} else {
		out.Param = def.Name
	}

	for _, v := range def.Leafs {
		fmt.Println("\t", v.Name)
	}
	fmt.Println("===============")

	for _, v := range def.Leafs {
		if v.Def != nil {
			parameter := JsonParameter{Name: v.Name, FullName: v.Def.Name, Type: v.Def.Type}
			out.JsonParameters = append(out.JsonParameters, parameter)
		}
	}

	if out.Name != "root" {
		*list = append(*list, out)
	}

	for _, v := range def.Leafs {
		if v.Def == nil {
			getJson(v, list, out.Param)
		}
	}
}

func getEnv(cfg *Config) []SetEnv {
	envs := []SetEnv{}

	for _, p := range cfg.Parameters {
		if len(p.Env) > 0 {
			envs = append(envs, SetEnv{Name: p.Name, Env: p.Env})
		}
	}
	return envs
}

func getDefault(cfg *Config) []SetDefault {
	defaults := []SetDefault{}

	for _, p := range cfg.Parameters {
		if len(p.Default) > 0 {
			defaults = append(defaults, SetDefault{Name: p.Name, Value: p.Default})
		}
	}
	return defaults
}

func getOpt(cfg *Config) []SetOpt {
	opts := []SetOpt{}

	for _, p := range cfg.Parameters {
		if len(p.ArgLong) > 0 && len(p.ArgShort) > 0 {
			et := "G_OPTION_ARG_STRING"
			d := "NULL"
			t := "gchar *"

			if p.Type == "boolean" {
				et = "G_OPTION_ARG_NONE"
				d = "FALSE"
				t = "gboolean "
			}

			opt := SetOpt{
				Name:      p.Name,
				Long:      p.ArgLong,
				Short:     p.ArgShort,
				Type:      t,
				Param:     p.FlatRef,
				Desc:      p.Desc,
				EntryType: et,
				Default:   d,
			}
			opts = append(opts, opt)
		}
	}
	return opts
}
func getParamOptions(opts []string, t string) (int, string) {
	if len(opts) == 0 {
		return 0, "NULL"
	}

	if t == "int" || t == "double" {
		return len(opts), "{ " + strings.Join(opts, ", ") + "}"
	}

	for i, v := range opts {
		opts[i] = "\"" + v + "\""
	}

	return len(opts), "{ " + strings.Join(opts, ", ") + "}"
}

func getCheckAndSet(cfg *Config) ([]string, []string) {
	var out []string
	var validators []string
	for _, p := range cfg.Parameters {
		var fn string
		if p.Type == "int" || p.Type == "string" || p.Type == "double" || p.Type == "size" {
			optc, optv := getParamOptions(p.Options, p.Type)
			vn := "NULL"
			if optc > 0 {
				vn = "valid_" + p.FlatRef
				switch p.Type {
				case "int":
					validator := "const gint64 " + vn + "[] = " + optv + ";"
					validators = append(validators, validator)
				case "double":
					validator := "const gdouble " + vn + "[] = " + optv + ";"
					validators = append(validators, validator)
				default:
					validator := "const gchar *" + vn + "[] = " + optv + ";"
					validators = append(validators, validator)
				}
			}

			fn = fmt.Sprintf("set_%s(\"%s\", g_hash_table_lookup(candidates, \"%s\"), &%s, %d, %d, %d, %s, err)", p.Type, p.Name, p.Name, structRef(p), p.Min, p.Max, optc, vn)
		}
		if p.Type == "boolean" {
			fn = fmt.Sprintf("set_%s(\"%s\", g_hash_table_lookup(candidates, \"%s\"), &%s, err)", p.Type, p.Name, p.Name, structRef(p))
		}
		if p.Type == "enum" {
			fn = fmt.Sprintf("set_%s_%s(\"%s\", g_hash_table_lookup(candidates, \"%s\"), &%s, err)", p.Type, p.FlatRef, p.Name, p.Name, structRef(p))
		}
		out = append(out, fn)
	}

	return out, validators
}

func getClear(cfg *Config) []string {
	var out []string
	for _, p := range cfg.Parameters {
		if p.Type == "string" {
			out = append(out, structRef(p))
		}
	}

	return out
}

func getOutput(cfg *Config) string {
	format := ""
	params := ""

	for _, v := range cfg.Parameters {
		format += "\"" + v.Name
		switch v.Type {
		case "int":
			format += ": %ld"
			params += ",\n  " + structRef(v)
		case "size":
			format += ": %ld"
			params += ",\n  " + structRef(v)
		case "double":
			format += ": %lf"
			params += ",\n  " + structRef(v)
		case "string":
			format += ": %s"
			params += ",\n  " + structRef(v)
		case "boolean":
			format += ": %d"
			params += ",\n  " + structRef(v)
		case "enum":
			format += ": %s"
			params += fmt.Sprintf(",\n  config_name_enum_%s(%s)", v.FlatRef, structRef(v))
		}
		format += "\\n\"\n"

	}
	return format + params
}

func getEnums(cfg *Config) []Enum {
	var res []Enum
	for _, v := range cfg.Parameters {
		if v.Type != "enum" {
			continue
		}

		e := Enum{Name: v.Name, FlatRef: v.FlatRef, EnumName: "config_" + v.FlatRef, Options: []EnumOption{}}

		for _, o := range v.Options {
			name := strings.ToUpper(v.Name + "_" + o)
			name = strings.Replace(name, ".", "_", -1)
			name = strings.Replace(name, "-", "_", -1)
			name = strings.Replace(name, " ", "_", -1)
			e.Options = append(e.Options, EnumOption{NiceName: o, EnumName: name})
		}
		res = append(res, e)
	}

	return res
}

func mapOutput(cfg *Config, def, json *Tree) *Output {
	output := Output{}
	output.Definitions = getDefinition(def, []Definition{})
	output.SetEnv = getEnv(cfg)
	output.SetDefault = getDefault(cfg)
	output.SetOpt = getOpt(cfg)
	output.CheckAndSet, output.ValidateOptions = getCheckAndSet(cfg)
	output.Clear = getClear(cfg)
	output.Enums = getEnums(cfg)
	getJson(json, &output.JsonObjects, "")
	output.OutputFormat = getOutput(cfg)

	return &output
}

func GetCFiles(cfg *Config, def, json *Tree) (string, string) {
	t, err := template.ParseFS(templateFiles, "templates/config.c.tmpl", "templates/config.h.tmpl")

	if err != nil {
		panic(err)
	}

	var cfile bytes.Buffer
	var hfile bytes.Buffer

	output := mapOutput(cfg, def, json)

	err = t.ExecuteTemplate(&cfile, "config.c.tmpl", output)
	if err != nil {
		panic(err)
	}

	err = t.ExecuteTemplate(&hfile, "config.h.tmpl", output)
	if err != nil {
		panic(err)
	}

	return cfile.String(), hfile.String()
}
