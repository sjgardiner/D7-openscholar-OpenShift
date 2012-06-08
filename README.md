Drupal on OpenShift
===================

This git repository helps you get up and running quickly w/ a Drupal installation
on OpenShift.  The backend database is MySQL and the database name is the
same as your application name (using $_ENV['OPENSHIFT_APP_NAME']).  You can name
your application whatever you want.  However, per default the name of the database
will always match the application so you might have to update *.openshift/action_hooks/deploy*
and *php/sites/default/settings.php*.


Running on OpenShift
--------------------

Create an account at http://openshift.redhat.com/

Create a php-5.3 application

    rhc app create -a drupal -t php-5.3

Add MySQL support to your application

    rhc app cartridge add -a drupal -c mysql-5.1

Add this upstream drupal repo

    cd drupal
    git remote add upstream -m master git://github.com/openshift/drupal-example.git
    git pull -s recursive -X theirs upstream master

    # note: if you named your application something other than 'drupal' make sure to edit
    #       php/sites/default/settings.php and modify the database to match the name
    #       of your application.

Then push the repo upstream

    git push

That's it, you can now checkout your application at:
    http://drupal-$yournamespace.rhcloud.com

Default Admin Username: Admin
Default Password: OpenShiftAdmin


Updates
-------

In order to update or upgrade to the latest drupal, you'll need to re-pull
and re-push.

Pull from upstream:

    cd drupal/
    git pull -s recursive -X theirs upstream master

Push the new changes upstream

    git push


Repo layout
-----------

php/ - Externally exposed php code goes here
libs/ - Additional libraries
misc/ - For not-externally exposed php code
../data - For persistent data
deplist.txt - list of pears to install
.openshift/action_hooks/build - Script that gets run every push, just prior to
    starting your app


Notes about layout
------------------

Please leave php, libs and data directories but feel free to create additional
directories if needed.

Note: Every time you push, everything in your remote repo dir gets recreated
please store long term items (like an sqlite database) in ../data which will
persist between pushes of your repo.


deplist.txt
-----------

A list of pears to install, line by line on the server.  This will happen when
the user git pushes.

Security Considerations
-----------------------
This repository contains configuration files with security related variables.

Since this is a shared repository, any applications derived from it will share those variables, thus reducing the security of your application.

You should follow the directions below and push your updated files to OpenShift immediately.

### Procedure

The following table lists files and variables that should be changed.

These values can be replaced by using the following script (as defined in `php/includes/bootstrap.inc`)

```php
function drupal_hash_base64($data) {
  $hash = base64_encode(hash('sha256', $data, TRUE));
  // Modify the hash so it's safe to use in URLs.
  return strtr($hash, array('+' => '-', '/' => '_', '=' => ''));
}
```

<table>
  <tr>
    <th>File</th>
    <th>Variable</th>
  </tr>
  <tr>
    <td>php/sites/default/settings.php</td> 
    <td>$drupal_hash_salt</td>
  </tr>
</table>


Additional information
----------------------

Link to additional information will be here, when we have it :)
