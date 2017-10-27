# Datawire notes -- Flynn <flynn@datawire.io>

**THIS WILL ALL GO AWAY** after the upstream merge. But it's handy for now.

## Git Wrangling

Set up your upstream with

```git remote add -t master upstream git@github.com:lyft/envoy.git```

Pull any upstream changes in with (from your `master` branch)

```
git fetch upstream
git merge upstream/master
git push
```

## Setup

Everything we use for Datawire's build is in the DATAWIRE directory, so get used to `cd`'ing there:

```cd DATAWIRE```

When you get to the testing part of the world, you'll need Python 3 with Flask installed.

## Building

Be in the DATAWIRE directory, then

```sh run_builder_bash.sh```

will fire up a Docker container with the build environment and give you a shell. If you exit out of it,

```docker start -i -a envoy-build```

will restart it with state preserved.

Inside the container, the whole `datawire/envoy` tree from the host is mounted on `/xfer` -- however, on a Mac at least, just running the build in `/xfer` is glacially slow. Instead, we have scripts to rsync the sources from the host into the container's disk, and build there. 

To do that:

```
cd ~/envoy
sh /xfer/DATAWIRE/go.sh
```

Once the build is finished, `go.sh` will copy the final result out to `/xfer/ci/envoy-static-build`, and you'll see it from the host side as `ci/envoy-static-build` in your `datawire/envoy` directory.

## Testing

Once you have a good build, open a new window, get back to the `DATAWiRE` directory, and run

```python simple-auth-server.py```

**Note well**: do this from the host, not the container. You'll need Python 3 and Flask.

Once the auth service is running, back in the container (from `~/envoy`) do

```sh DATAWIRE/envoy-test.sh```

to start a test envoy listening on port 9999 (the Envoy config is in `envoy-test.json`).

At that point, you can use curl to see how things are going. It shouldn't matter whether you do these from inside or outside the container:

```curl -v http://localhost:9999/bad1/```
- auth should fail the request
- curl status code should be 403

```curl -v http://localhost:9999/bad2/```
- auth should fail the request
- curl status code should be 403

```curl -v http://localhost:9999/nohdr/```
- auth should succeed, but add no headers, so Envoy shouldn't match anything
- curl status code should be 404

```curl -v http://localhost:9999/good/```
- auth should succeed and add the x-hurkle header, so Envoy should route this
- curl should show the httpbin page

## Formatting

Envoy is draconian about formatting. Once you're feeling good about the build, from inide the container you can run

```python tools/check_format.py fix```

to make sure the source formatting matches Envoy's draconian rules. You'll have to copy any changed files out to `/xfer` by hand, sadly.

## Dockerizing

Back on the host, after you have a good build:

```
docker build -f ci/Dockerfile-envoy-unstripped -t datawire/ambassador-envoy:$(git describe --tags) ci
docker push datawire/ambassador-envoy:$(git describe --tags)
```

(Obviously, if you're not Datawire, you should change the Docker registry in use. `datawire` won't work for you.)

