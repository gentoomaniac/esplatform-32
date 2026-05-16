package main

import (
	"bytes"
	"encoding/json"
	"fmt"
	"net/netip"

	"github.com/alecthomas/kong"
	"github.com/rs/zerolog/log"

	gocli "github.com/gentoomaniac/iot/pkg/cli"
	"github.com/gentoomaniac/iot/pkg/logging"
	"github.com/gentoomaniac/iot/pkg/request"
)

var (
	version = "unknown"
	commit  = "unknown"
	binName = "unknown"
	builtBy = "unknown"
	date    = "unknown"
)

var cli struct {
	logging.LoggingConfig

	Address netip.Addr `short:"a" help:"target address of the device" required:""`
	Rpc     string     `short:"r" help:"rpc method to call" required:""`
	Data    string     `short:"d" help:"data to be sent in json"`

	User     string `short:"u" help:"authentication username" default:"admin"`
	Password string `short:"p" help:"authentication password"`

	Pretty bool `help:"pretty print json response" default:"false"`

	Version gocli.VersionFlag `short:"V" help:"Display version."`
}

func main() {
	ctx := kong.Parse(&cli, kong.UsageOnError(), kong.Vars{
		"version": version,
		"commit":  commit,
		"binName": binName,
		"builtBy": builtBy,
		"date":    date,
	})

	logging.Setup(&cli.LoggingConfig)
	resp, err := request.SendRequest(cli.Address, request.RPCMethod(cli.Rpc), cli.Data, &request.Auth{Username: cli.User, Password: cli.Password})
	if err != nil {
		log.Fatal().Err(err).Msg("failed sending request")
	}

	if cli.Pretty {
		var prettyJSON bytes.Buffer
		err := json.Indent(&prettyJSON, resp, "", "  ")
		if err != nil {
			log.Fatal().Err(err).Msg("failed prettyfying json response")
		}
		fmt.Println(prettyJSON.String())
	} else {
		fmt.Println(string(resp))
	}

	ctx.Exit(0)
}
