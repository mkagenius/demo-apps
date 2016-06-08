# demonstration application

This is our January 2016 top neural network for large-scale object recognition. It has been trained to recognize most typical home indoor/outdoor objects in our daily life. It was trained with more that 32 M images on a private dataset. It can serve as good pair of eyes for your machines, robots, drones and all your wonderful creations!

<!--[![icon](icon.jpg|width=400px)]-->
<a href="icon"><img src="icon.jpg" align="center" height="400" width="400" ></a>

See it in action in this [video #1](https://www.youtube.com/watch?v=_wXHR-lad-Q), and also this other [video #2](https://www.youtube.com/watch?v=B0TreumQO-0).

This application is for tinkerers, hobbiest, researchers, evaluation purpose, non-commercial use only.

It has been tested on OS X 10.10.3 and Linux. It can run at > 17 fps on a MacBook Pro (Retina, 15-inch, Late 2013) on CPU only.


# Install for Mac OSX:

## Have a wired connection or a nice WIFI connection before starting.

* Install Torch7: Visit http://torch.ch/ (click on `get started`), btw, steps copied from this site below:
  ```
  git clone https://github.com/torch/distro.git ~/torch --recursive
  cd ~/torch; bash install-deps;
  ./install.sh
  source ~/.bash_profile (generally this, or whatever is the bash profile file)
  ```

* Please download files: `model.net`, `categories.txt` and `stat.t7` from:
* `https://www.dropbox.com/sh/u3bunkfm0dzjix6/AABQ4Nq4-70MU57MxXyyjrrMa?dl=0` (This is our new 5000 categories network from January 2016.)
* Place them in folder in `generic-pc` folder, all three of them.
* ~~Linux camera install: `cd lib/` then `make; make install`. Note that `Makefile` wants Torch7 installed in `/usr/local/bin`, otherwise please change accordingly!~~ (not required for OSX)
 
## Install `opencv` if not there
 * `brew update; brew update; brew tap homebrew/science; brew install opencv` (execute one by one if fails)
 * Follow the instruction after the `opencv` install is done, something like the following two:
 * `mkdir -p /Users/<username>/Library/Python/2.7/lib/python/site-packages`
 * `echo 'import site; site.addsitedir("/usr/local/lib/python2.7/site-packages")' >> /Users/<username>/Library/Python/2.7/lib/python/site-packages/homebrew.pth`
 
## Install Mac Camera
 * `luarocks install camera`



## Run:
* To run with a webcam and display on local machine: ```qlua run.lua```

* Zoom window by 2 (or any number): ```qlua run.lua -z 2```


That's it, it should open up a small window with your webcam showing your face (maybe classifying it as `Marshall` as in my case)
