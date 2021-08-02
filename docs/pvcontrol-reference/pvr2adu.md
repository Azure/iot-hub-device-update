# pvr2adu

The script is located in ```tools/pvr2adu``` and must be used in the same path of the [pvr checkout](https://docs.pantahub.com/make-a-new-revision.html) that we want to convert to ADU format. Some parameters must be set for the json manifest:

```
$ tools/pvr2adu 
Usage: pvr2adu [-h] -p <PROVIDER> -n <NAME> -m <MANUFACTURER> -d <MODEL> -v <VERSION> -o <OUTPUT>
Creates ADU compatible manifest and tarball from current directory
       options:
              -h      show help
              -p      provider
              -n      name
              -m      manufacturer
              -v      version
              -o      output
```
