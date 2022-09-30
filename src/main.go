package main

import (
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"reflect"
	"strings"

	"gopkg.in/yaml.v2"
)

type Parameter struct {
	Name     string   `yaml:"name" unique:"true"`
	Desc     string   `yaml:"description"`
	Default  string   `yaml:"default"`
	Json     string   `yaml:"json" unique:"true"`
	Env      string   `yaml:"env" unique:"true"`
	ArgLong  string   `yaml:"arg-long" unique:"true"`
	ArgShort string   `yaml:"arg-short" unique:"true"`
	Type     string   `yaml:"type"`
	Min      int      `yaml:"min"`
	Max      int      `yaml:"max"`
	Options  []string `yaml:"options"`
	FlatRef  string
}

type Config struct {
	Parameters []Parameter `yaml:"parameters"`
}

type Tree struct {
	Name  string
	Leafs map[string]*Tree
	Def   *Parameter
}

func add_to_tree(tree *Tree, param string, def *Parameter) {
	parts := strings.Split(param, ".")
	var ok bool
	var leaf *Tree
	for _, p := range parts {
		leaf, ok = tree.Leafs[p]
		if !ok {
			leaf = new(Tree)
			leaf.Leafs = make(map[string]*Tree)
			tree.Leafs[p] = leaf
			leaf.Name = p
		}
		tree = leaf
	}

	tree.Def = def
}

func getReadme(cfg *Config) string {
	md := "| Name | Short arg | Long arg | Env | Json conf | Description |\n"
	md += "------ | --------- | -------- | --- | --------- | ----------- |\n"

	for _, p := range cfg.Parameters {
		md += fmt.Sprintf("%s | %s | %s | %s |%s | %s\n",
			p.Name, p.ArgShort, p.ArgLong, p.Env, p.Json, p.Desc)
	}

	return md
}

func validateUnique(list []Parameter) error {
	values := make(map[string]bool)

	for _, p := range list {
		uv := reflect.ValueOf(p)
		ut := uv.Type()

		for i := 0; i < ut.NumField(); i++ {
			field := ut.Field(i)
			if uv.Field(i).String() == "" {
				continue
			}

			if _, ok := field.Tag.Lookup("unique"); ok {
				str := field.Name + ":" + uv.Field(i).String()
				if _, exists := values[str]; exists {
					return fmt.Errorf("%s has duplicate: %s", field.Name, uv.Field(i).String())
				}
				values[str] = true
			}
		}
	}

	return nil
}

func main() {
	var cfg Config
	var json Tree
	var def Tree

	filename, _ := filepath.Abs("./config-meta.yml")
	yamlFile, err := ioutil.ReadFile(filename)

	def.Leafs = make(map[string]*Tree)
	json.Leafs = make(map[string]*Tree)

	def.Name = "config"
	json.Name = "root"

	err = yaml.Unmarshal(yamlFile, &cfg)
	if err != nil {
		panic(err)
	}

	uniqueErr := validateUnique(cfg.Parameters)
	if uniqueErr != nil {

		fmt.Println(uniqueErr)
		os.Exit(1)
	}

	for i, p := range cfg.Parameters {
		fmt.Println(p.Name)
		cfg.Parameters[i].FlatRef = fmt.Sprintf("%s", strings.ReplaceAll(p.Name, ".", "_"))

		add_to_tree(&def, p.Name, &cfg.Parameters[i])
		add_to_tree(&json, p.Json, &cfg.Parameters[i])
	}

	fmt.Println("======== NEW ===========")

	cfile, hfile := GetCFiles(&cfg, &def, &json)

	cout, err := os.Create("testapp/config.c")

	if err != nil {
		fmt.Println(err)
	} else {
		cout.WriteString(cfile)
	}

	cout.Close()

	hout, err := os.Create("testapp/config.h")

	if err != nil {
		fmt.Println(err)
	} else {
		hout.WriteString(hfile)
	}

	hout.Close()

	fmt.Printf(getReadme(&cfg))

}
