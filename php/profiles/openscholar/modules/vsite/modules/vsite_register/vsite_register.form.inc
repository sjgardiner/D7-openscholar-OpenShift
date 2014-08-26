<?php

/**
 * @function vsite_site_register_form
 *
 * Provides form for site/register
 */
function vsite_register_form($form, &$state) {
  global $base_url;
  global $user;
  $form = array();
  $anonymous = ($user->uid == 0);
  $manager = (user_access('create vsite content on behalf') && user_access('administer users'));

  $parent = null;
  if (isset($_GET['parent']) && $parent = vsite_get_vsite($_GET['parent'])) {
    $parent = $_GET['parent'];
  }
  $form['parent'] = array(
    '#type' => 'hidden',
    '#value' => $parent,
  );

  $form['domain'] = array(
    '#type' => 'textfield',
    '#title' => t('Your URL'),
    '#required' => TRUE,
    '#field_prefix' => $base_url . '/',
    '#size' => 35,
    '#description' => t('Minimum of 3 characters, which must be lowercase. Punctuation is not allowed except for hyphens and underscores. <br />Example: a choice of \'jdoe\' would result in the URL: %site.', array('%site' => $base_url . '/jdoe')),
    '#ajax' => array(
      'callback' => 'vsite_register_ajax_validation',
      'wrapper' => 'domain-errors',
    ),
    '#field_suffix' => theme('vsite_register_form_errors', array('name'=>'domain')),
    '#attached' => array('js' => array(drupal_get_path('module', 'vsite_register') . '/vsite_register.js')),
  );

  $group_types = og_get_all_group_bundle();
  $default_bundle = variable_get('os_default_group_bundle', 'personal');
  $bundles = array();
  foreach ($group_types['node'] as $bundle => $label) {
    if (user_access("create {$bundle} content") && (!isset($parent) || in_array($bundle, array('personal', 'project')))) {
      $bundles[$bundle] = $label;
    }
  }
  if(!isset($bundles[$default_bundle])){
    $default_bundle = current($bundles);
  }

  $form['bundle'] = array(
    '#title' => t('Type of site'),
    '#type' => 'select',
    '#required' => TRUE,
    '#options' => $bundles,
    '#default_value' => $default_bundle,
  );

  if (count($bundles) <= 1) {
    $form['bundle']['#type'] = 'value';
    $form['bundle']['#value'] = $default_bundle;
  }else{
    $form['bundle']['#ajax'] = array(
      'event' => 'change',
      'wrapper' => 'preset-wrapper',
      'callback' => 'vsite_register_ajax_avalible_presets',
      'method' => 'replace',
    );
  }

  $selected_bundle = isset($state['values']['bundle']) ? $state['values']['bundle'] : $default_bundle;

  //Get enabled presets and load their titles
  $enabled_presets = variable_get('os_enabled_spaces_presets', array());
  $presets = spaces_preset_load();
  foreach($enabled_presets as $key => $value) {
    $enabled_presets[$key] = $presets[$key]->title;
    //Remove presets that don't go with this bundle
    if(isset($presets[$key]->value['bundles']) && !in_array($selected_bundle, $presets[$key]->value['bundles'])){
     unset($enabled_presets[$key]);
    }
  }
  $default_preset = variable_get('spaces_preset_og', 'os_scholar');

  $form['preset'] = array(
    '#prefix' => '<div id="preset-wrapper">',
    '#suffix' => '</div>',
    '#title' => t('Initial Setup'),
    '#type' => 'select',
    '#required' => TRUE,
    '#options' => $enabled_presets,
    '#default_value' => (isset($enabled_presets[$default_preset])) ? $default_preset : current(array_keys($enabled_presets)),
  );

  if (count($enabled_presets) <= 1) {
    $form['preset']['#type'] = 'value';
    $form['preset']['#value'] = $form['preset']['#default_value'];
  }

  $form['vsite_private'] = array(
    '#title' => t('Private Site'),
    '#type' => 'checkbox',
    '#default_value' => FALSE,
    '#description' => t('Make your site private.  It won\'t be visible to anyone but you and won\'t be indexed by Google until you make it public.')
  );


  if ($anonymous) {
    //anonymous users always create new accounts
    $form += _vsite_register_user_form();
    $form['vicarious_user'] = array(
      '#type' => 'hidden',
    	'#value' => FALSE,
    );
    //flag for JS to set when create new user is clicked
    $form['user_fieldset']['create_new_user'] = array(
      '#type' => 'hidden',
      '#default_value' => TRUE,
    );

  } elseif($manager) {
    //managers can create a new site, give a site to another user, or give a site to a new account.
    $form['vicarious_user'] = array(
      '#type' => 'checkbox',
    	'#title' => t('Create this OS Site on behalf of another user.'),
    );

    $form['user_fieldset'] = array(
      '#type' => 'fieldset',
      '#collapsible' => FALSE,
    	'#states' => array('visible' => array(':input[name="vicarious_user"]' => array('checked' => TRUE))),
    );

    //$ajax_link = l(t('create a new user'), 'javascript:void(0)', array('attributes'=>array('id'=>array('new-user-link'))));
    $ajax_link = '<a href="javascript:void(0)" id="new-user-link">' . t('create a new user') . '</a>'; //figure out l() later

    $form['user_fieldset']['existing_username'] = array(
      '#type' => 'textfield',
    	'#title' => t('Find Existing User'),
      '#description' => t('Enter the name of the user who will manage this new site or !create_new.',
        array('!create_new' => $ajax_link)
      ),
    	'#autocomplete_path' => 'user/autocomplete',
    );

    //flag for JS to set when create new user is clicked
    $form['user_fieldset']['create_new_user'] = array(
      '#type' => 'hidden',
      '#default_value' => FALSE,
    );

    $form['user_fieldset']['new_user_div'] = array(
      '#markup' => '<div id="new_user_div">',
    );

    $form['user_fieldset'] += _vsite_register_user_form();

    $form['user_fieldset']['huid'] = array();

    $form['user_fieldset']['new_user_close_div'] = array(
      '#markup' => '</div>',
    );

  } else {
    //regular users can only create sites for themselves.
    $form['vicarious_user'] = array(
      '#type' => 'value',
      '#value' => FALSE,
    );
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Create your site'),
    '#weight' => 40,
    '#prefix' => '<div id="form-errors"></div>',
    '#suffix' => '<div id="submit-suffix"></div>',
    '#attributes' => array('class' => array('use-ajax-submit') ),
    '#ajax' => array('callback' => 'vsite_register_form_submit'),
  );

  return $form;

}

//returns user section of vsite_register form
function _vsite_register_user_form() {
  $form = array();
  $form['name'] = array(
    '#type' => 'textfield',
    '#size' => 35,
    '#title' => t('Desired user name'),
    '#description' => t('Spaces are allowed; punctuation is not allowed except for periods, hyphens, and underscores.'),
    '#ajax' => array(
      'callback' => 'vsite_register_ajax_validation',
      'wrapper' => 'name-errors',
    ),
    '#field_suffix' => theme('vsite_register_form_errors', array('name'=>'name')),
  );

  $form['first_name'] = array(
    '#type' => 'textfield',
    '#size' => 35,
    '#title' => t('First name'),
  );

  $form['last_name'] = array(
    '#type' => 'textfield',
    '#size' => 35,
    '#title' => t('Last name'),
  );

  $form['mail'] = array(
    '#type' => 'textfield',
    '#size' => 35,
    '#title' => t('Email address'),
    '#description' => t('A valid e-mail address is required. All e-mails from OpenScholar will be sent to this address.'),
    '#ajax' => array(
      'callback' => 'vsite_register_ajax_validation',
      'wrapper' => 'mail-errors',
    ),
    '#field_suffix' => theme('vsite_register_form_errors', array('name'=>'mail')),
  );

  // password field
  $form['password'] = array(
    '#type' => 'password_confirm',
    '#size' => 35,
  );
  /*
  $form['notify'] = array(
    '#type' => 'checkbox',
    '#title' => t('Send email notification of this account.'),
  );
  */
  return $form;
}



/**
 * @function vsite_site_register_form
 *
 * Creates user and vsite when site/register form is submitted
 */
function vsite_register_form_submit(&$form, &$state) {
  ctools_include('user', 'os');
  ctools_include('vsite', 'vsite');
  $values = $state['values'];

  if ($values['vicarious_user'] && $values['existing_username']) {
    //user account already exists
    $site_owner = user_load_by_name($values['existing_username']);

  } elseif (($values['vicarious_user'] && !$values['existing_username']) || (!$values['vicarious_user'] && $values['name'])) {
    // create user for self or on someone's behalf
    $user_options = array(
    	'name' => $values['name'],
    	'pass' => $values['password'],
    	'mail' => $values['mail'],
    	'status' => 1,
      'field_first_name' => $values['first_name'],
      'field_last_name' => $values['last_name'],
    );
    $site_owner = os_user_create($user_options);

    // Send out an email to notify the user of the newly created account.
    // Email only sent if user requested to be notified of new account.
    // if ($values['notify']) {
    //  _user_mail_notify('register_no_approval_required', $site_owner);
    // }

    //log in as new user, if we're not already logged in.
    global $user;
    if ($user->uid == 0) {
      $user = $site_owner;
      user_login_finalize($state);
    }

  } else {
    //creating site for self.  no need to create a new user
    global $user;
    $site_owner = $user;
  }

  //create the vsite
  if ($vsite = vsite_create_vsite($values['domain'], $values['domain'], $site_owner->uid, $values['bundle'], $values['preset'], $_POST['parent'])) {
    //add user to vsite
    module_load_include('inc', 'vsite', 'includes/user');
    vsite_user_add($site_owner);

    os_role_grant($site_owner->uid, 'vsite admin', $vsite->id);

    $message = vsite_register_message($form, $values['domain']);
    $commands[] = ajax_command_replace ('#submit-suffix', $message );
    $commands[] = ajax_command_remove('#edit-submit');

    if (isset($values['vsite_private']) && $values['vsite_private'] && module_exists('vsite_access')) {
      $node = node_load($vsite->group->nid);
      $node->group_access[LANGUAGE_NONE][0]['value'] = 1;
      node_save($node);
    }

    // Email user to inform them of new created site, email is always sent.
    vsite_register_notify_user($vsite);

    //it worked, clear the errors
    drupal_get_messages('error');


  } else {
    $commands[] = _vsite_register_form_error();
  }

  //call any other submit functions here
  foreach($form['#submit'] as $fnc){
    if (__FUNCTION__ !== $fnc){
      call_user_func($fnc, $form, $state);
    }
  }
  print ajax_render($commands);
  exit;
}


/**
 * @function vsite_site_register_form_validate
 *
 * Validation function for site/register form
 */
function vsite_register_form_validate(&$form, &$state) {
  //Ajax validation.
  if ($_GET['q'] == 'system/ajax') {
    if ($state['triggering_element']['#ajax']['callback'] == 'vsite_register_ajax_validation' ||
        $state['triggering_element']['#ajax']['callback'] == 'iqss_pinserver_register_ajax_validation') {
      //ajax callback will validate just this element and return a message.
      return;
    } else if ($state['triggering_element']['#ajax']['callback'] != 'vsite_register_ajax_avalible_presets'){
      //form is done.  maybe this should happen after the validate?  or at least duplicate its efforts.
      return vsite_register_ajax_submit_validation($form, $state);
    }
  }

  //Fall back to standard validation
  $values = $state['values'];
  foreach (vsite_register_validate_elements($values) as $element => $msg) {
    if ($msg) {
      form_set_error($element, $msg);
    }
  }

  if ($values['create_new_user']) {
    //tell javascript to switch to new user form instead of making user switch it again.
    drupal_add_js(array('vsite_register' => array('new_user_clicked' => TRUE)), 'setting');
  }
}

/**
 * @function vsite_register_ajax_validation
 *
 * Callback function for ajax.  returns errors on ajaxy elements
 */
function vsite_register_ajax_validation(&$form, &$state) {
  drupal_get_messages(); //clear this to avoid having notices clutter the form during ajax validation
  $elem = $state['triggering_element']['#name'];
  return theme('vsite_register_form_errors', (array(
    'name' => $elem,
    'errors' => _vsite_register_validate($elem, $state['values']),
  )));
  //$errors = form_get_errors();//$state['triggering_element']['#name']);
}

/**
 * Update the avalible presets
 *
 * @param $form
 * @param $form_state
 * @return $form
 */
function vsite_register_ajax_avalible_presets($form, $form_state) {

  //Hard work is done in the form build function, just return the item
  return $form['preset'];
}

/**
 * @function vsite_register_ajax_submit_validation
 *
 * Called when submit button is clicked and ajax is enabled.
 */
function vsite_register_ajax_submit_validation(&$form, &$state) {
  $values = $state['values'];
  $valid = $commands = array();

  foreach (vsite_register_validate_elements($values) as $element => $msg) {
    if ($msg) {
      $html = theme('item_list', array('items'=>array($msg), 'type'=>'ul', 'attributes'=>array('class'=>array('error'))));
      $commands[] = ajax_command_html("#${element}-errors", $html);
    } else {
      $valid[] = ajax_command_html("#${element}-errors", '');
    }
  }

  //print new errors
  if (count($commands)) {
    print ajax_render(array_merge($commands, $valid));
    exit;
  }
}

/**
 * @function vsite_register_validate_elements
 *
 * Validates form elements, returns errors.  Errors can be handled as usual or ajax rendered.
 *
 * Exists as separate function because there's more logic than usual in choosing which elements need to validate
 * based on the state of the form.
 *
 * @param array $values
 *   Form state's values array
 * @return array
 *   element => error
 */
function vsite_register_validate_elements($values) {
  //always check domain and form
  $elements = array('domain', 'form');

  //new user
  if ($values['create_new_user']) {
    $elements = array_merge($elements, array('name', 'mail', 'password'));
  }

  //existing user
  if ($values['vicarious_user'] && !$values['create_new_user']) {
    $elements = array_merge($elements, array('existing_username'));
  }

  $msgs = array();
  foreach ($elements as $element) {
    if ($errors = _vsite_register_validate($element, $values)) {
      $msgs[$element] = implode(' ', $errors);
    }
  }

  return $msgs;
}

/**
 * @function _vsite_register_validate
 *
 * vsite_register_ajax_validation and vsite_register_form_validate are wrappers for this function.
 * Common elements are validated here.
 *
 * @param string $element
 * 	Name of element to validate
 *
 * @param array $values
 * 	values array from returned form state
 *
 * @return string listing all errors with form element.  Null if no errors.
 */
function _vsite_register_validate($element, &$values) {
  $msg = array();
  switch($element) {

    case 'domain':
      if (strlen($values['domain']) < 3 || !valid_url($values['domain']) || !_vsite_register_valid_url($values['domain'])) {
        $msg[] = t('Invalid URL specified.');
      }
      if (($purl = purl_load(array('value'=>$values['domain'], 'provider'=>'spaces_og'), TRUE)) || menu_get_item($values['domain'])) {
        $msg[] = t('URL %url is taken.  Please choose another.', array('%url'=>$values['domain']));
      }

    break;

    case 'mail':
      if ($mail_error = user_validate_mail($values['mail'])) {
        $msg[] = $mail_error;
      }

      if (_vsite_register_mail_in_use($values['mail'])) {
        $msg[] = t('Email address already in use.  Please try another.');
      }
    break;

    case 'name':
      if ($user_error = user_validate_name($values['name'])) {
        $msg[] = $user_error ;
      }

      if ($user = user_load_by_name($values['name'])) {
        $msg[] =  t('Username %name is taken.  Please choose another.', array('%name' => $values['name']));
      }
    break;

    //check that all dependent fields are provided.
    case 'form':
      //on behalf selected, but no user specified.
      if ($values['vicarious_user'] && !(strlen($values['existing_username']) || $values['create_new_user'])) {
        $msg[] = t('You have selected to create this site on behalf of another user.  Please specify an existing user or create a new account.');
      }

      //new user
      if ($values['create_new_user']) {
        if (!(strlen($values['name']) && strlen($values['mail']) && strlen($values['password']))) {
          $msg[] = t('You have selected to create a new user.  Please provide a username, email address, and password for the new user.');
        }

        //confirm_password
        $pass1 = trim($_POST['password']['pass1']);
        $pass2 = trim($_POST['password']['pass2']);
        if (!empty($pass1) || !empty($pass2)) {
          if (strcmp($pass1, $pass2)) {
            $msg[] = t('The specified passwords do not match.');
          }
        } else {
          $msg[] = t('The password field is required.');
        }
      }

      //existing user
      if ($values['vicarious_user'] && !$values['create_new_user']) {
        if (!user_load_by_name($values['existing_username'])) {
          $msg[] = t('User @name does not exist.  Please try another user name or create a new user account instead.', array('@name'=>$values['existing_username']));
        }
      }
    break;

  }

  return (count($msg)) ? $msg : NULL;
}

/**
 * @function _vsite_mail_in_use
 *
 * Checks if an email address is already in use by another user.
 * Code borrowed from user.module's user_account_form_validate()
 *
 * @param string $mail
 * 	email address
 */
function _vsite_register_mail_in_use($mail) {
  return (bool) db_select('users')
              ->fields('users', array('uid'))
              ->condition('mail', db_like($mail, 'LIKE'))
              ->range(0, 1)
              ->execute()
              ->fetchField();
}

/**
 * @function _vsite_register_valid_url
 *
 * Checks a url for bad characters.
 *
 * @param string $url
 */
function _vsite_register_valid_url($url) {
  foreach (array('!', ';', '/', '?', ':', '@', '&', '=', '"', "'",) as $char) {
    if (strpos($url, $char) !== FALSE) {
      return FALSE;
    }
  }

  return purl_validate(array('value' => $url, 'provider' => 'spaces_og'));
}

/*
 * Function to send out an email to the user
 * to notify them of their newly created site.
 */
function vsite_register_notify_user($vsite){
  global $language;
  global $base_url;
  global $user;

  // Check to see if the user logged in matched the newly created user.
  if($user->uid == $vsite->group->uid) {
  	$need_pass_reset = FALSE;
  }else {
  	$need_pass_reset = TRUE;
  }

  $params = array();
  $from = variable_get('site_mail', ini_get('sendmail_from'));
  $new_user = user_load($vsite->group->uid);

  //add vsite and user object
  $params['vsite'] = $vsite;
  $params['user'] = $new_user;

  $scholar_site_url = url($base_url.'/'.$vsite -> group->title, $options = array('absolute' => TRUE));
  $count = count(vsite_get_vsite_by_owner($new_user -> uid));
  $timestamp = time();

  $subject = t("An OpenScholar web site has been created for you.", array(
    '!site_name' => variable_get('site_name', 'OpenScholar')
  ));
  $msg_variables = array(
    '!vsite-count' => $count,
    '!username' => $new_user->name,
    '!site' => variable_get('site_name', 'OpenScholar'),
    '!scholar_site_url' => $scholar_site_url,
    '!login_url' => user_pass_reset_url($new_user),
    '!site_login_uri' => url($base_url.'/'.$vsite -> group->title . '/home', array('absolute' => TRUE)),
    '!uri' => $base_url,
    '!uri_brief' => preg_replace('!^https?://!', '', $base_url),
    '!mailto' => $new_user->mail,
    '!date' => format_date(time(), 'medium', '', NULL, $message['language']->language),
    '!login_uri' => url('user', array(
      'absolute' => TRUE,
      'language' => $message['language']
    )),
    '!site_url' => url($vsite -> group -> purl),
    '!edit_uri' => url('user/' . $new_user->uid . '/edit', array(
      'absolute' => TRUE,
      'language' => $message['language']
    )),
    '!scholar_edit_url' => url("/user/{$new_user->uid}/edit", array('absolute' => TRUE)),
  );

  //detemine if user is creating first site or additional site, send appropriate message
  if(count(vsite_get_vsite_by_owner($new_user -> uid)) > 1 || $need_pass_reset == FALSE){
    $body = t("Hi !username,\n\nYour new web site at !site has been activated. Your web site url is !scholar_site_url\n\nYou will be able to log into your site at !site_login_uri with username:\n\n!username\n\nWe hope you will enjoy your new web site.\n\nSincerely,\nThe !site Team",
     $msg_variables);
  }else{
    $body = t("Hi !username,\n\nYour new web site at !site has been activated.\n\nYour web site url is !scholar_site_url\n\nYou may now log in and reset your password by clicking on this link or copying and pasting it in your browser: \n\n !login_url \n\nThis login can be used only once. After logging in, you will be redirected to !scholar_edit_url so you can change your password. Once you have set your own password, you will be able to log in at !site_login_uri as username:\n\n!username\n\nWe hope you will enjoy your new web site.\n\nSincerely,\nThe !site Team",
    $msg_variables);
  }

  $params['subject'] = $subject;
  $params['body'] = $body;
  $result = drupal_mail('vsite_register', 'vsite_register_notify', $new_user->mail, $language, $params, $from, $send=TRUE);
  if($result == FALSE) {
  	drupal_set_message(t('Error sending notification mail to user.'), 'error');
  }
}

/**
 * Implements hook_mail().
 */
function vsite_register_mail($key, &$message, $params) {
  $message['subject'] = $params['subject'];
  $message['body'][] = $params['body'];

  //these values be used with any mail alter hooks
  $message['vsite'] = $params['vsite'];
  $message['user'] = $params['user'];
}
