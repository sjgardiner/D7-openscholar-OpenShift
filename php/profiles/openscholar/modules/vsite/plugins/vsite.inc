<?php
// $Id$


/**
 * VSite space object
 *
 * @author rbrandon
 *
 * @todo Check Node Access
 * @todo Change Private Site handler
 * @todo Change Access Admin to work using permissions
 *
 */
class vsite extends space_og {

  // Stores the current provider.
  // Allows for a multi-provider system.
  public $purl_provider = FALSE;

  // Provides our own states different from the default Disabled/Enabled
  function feature_options() {
    return array(
      0 => t('Disabled'),
      1 => t('Public'),
      2 => 'Private',
    );
  }

  /**
   * Returns the absolute URL to this vsite
   */
  public function get_absolute_url($path = "") {
    // If this is an inactive site get the homepage path from the group node
    if ($this->active && !$this->purl_provider) {}
    elseif (!$this->purl_provider) {
      self::verify_purl();
    }

    return url($path, array(
        'purl' => array(
          'provider' => $this->purl_provider,
          'id' => $this->id,
        ),
        'absolute' => TRUE,
      ));
  }

  /**
   * Overrides activate().
   */
  function activate() {
    // Test purl paths
    $this->activate_purl_modifier();

    if (parent::activate()) {
      // Sets up user roles (call before menu_get_item caches access info)
      $this->activate_user_roles();

      $router_items = &drupal_static('menu_get_item');
      // Have the menu access params already been set?
      if (isset($router_items[$_GET['q']])) {
        // If the current menu item may rely on the space so we reset the cached access information
        unset($router_items[$_GET['q']]);
      }

      return TRUE;
    }

    return FALSE;
  }

  /**
   * Implements space->router().
   */
  public function router($op, $object = NULL) {
    global $user;
    switch ($op) {
      case 'node':
        // If we are viewing the group node
        if (og_is_group_type('node', $object->type)) {
          if (isset($object->nid) && (!(isset($this->active) && $this->active) || ($this->active && $this->id != $object->nid)) && $space = spaces_load('og', $object->nid)) {
            // Sets up the correct purl provider
            $space->verify_purl();
            $current_url = url($_GET['q'], array(
                'absolute' => TRUE,
                'query' => drupal_get_query_parameters(),
              ));
            // Goto vsite's home <front> page
            if ($current_url != rtrim($space->get_absolute_url(), "/")) {
              purl_goto("<front>", array(
                  'purl' => array(
                    'provider' => $space->purl_provider,
                    'id' => $space->id,
                  ),
                ));
            }
          }
        }
        break;

      case 'init':
        // Additional check for private sites.
        if ($this->active && module_exists('vsite_access') && !vsite_access_user_access($this->group, $user)) {
          $provider = array('provider' => $this->purl_provider, 'id' => $this->id);
          purl_goto('private_site', array(
              'purl' => $provider,
              'query' => array(
                'destination' => url('home', array(
                    'purl' => $provider,
                  )),
              ),
            )
          );
        }
        // Checks group content for extraneous purl on custom domain requests.
        else if ($object = menu_get_object()) {
          if (isset($object->nid) && !og_is_group_type('node', $object->type)) {
            $items = field_get_items('node', $object, OG_AUDIENCE_FIELD);
            // First, redirects if this node does not belong to this vsite.
            if ($items[0]['target_id'] && ((int) $items[0]['target_id'] !== (int) $this->id)) {
              purl_goto(drupal_lookup_path('alias', "node/$object->nid"), array(), 301);
            }
            // Now, redirects to path without purl, if we are on a vsite with
            // a custom domain and original path contains the purl.
            else if ($items[0]['target_id'] && $this->purl_provider == 'vsite_domain') {
              $request_path_args = explode('/', request_path());
              $index = array_shift($request_path_args);
              if (!empty($request_path_args) && ((string) $index === (string) $this->group->purl)) {
                $path_without_purl = implode('/', $request_path_args);
                purl_goto($path_without_purl, array(), 301);
              }
            }
          }
        }
        break;
    }
    parent::router($op, $object);
  }

  /**
   * Overrides access_feature().
   */
  function access_feature($op = 'view', $feature, $account = NULL) {
    if ($feature == 'vsite' && $op == 'view' && user_access('access content')) {
      return TRUE;
    }

    $access = parent::access_feature($op, $feature, $account);

    // Checks feature 'privacy' settings
    $features = variable_get('spaces_features', array());
    if ($access && $features[$feature] > 1) {
      // Handles privacy levels that are not simple Public/Disabled
      switch ($features[$feature]) {
        // Private
        case 2:
          // Admin or group member?
          if (!og_is_member($this->group_type, $this->id, 'user', $account) && !user_access('administer nodes')) {
            $access = FALSE;
          }
          break;
        // Unknown Privacy Level
        default:
          $access = FALSE;
          break;
      }
    }

    // Additional check for private sites.
    if ($access && module_exists('vsite_access')) {
      return $access && vsite_access_user_access($this->group, $account);
    }

    return $access;
  }

  /**
   * verify_purl()
   *
   * If this space is loaded outside a active context (ie externally from the
   * home page), then determine which purl_provider would be best domain if they
   * have that set otherwise path.
   */
  protected function verify_purl() {
    $domain_method = variable_get('purl_method_vsite_domain', FALSE);
    if ($domain_method) {
      $elements = purl_active()->get($domain_method);
      foreach ($elements as $element) {
        if ($element->provider == "vsite_domain" && $element->id == $this->id) {
          $this->purl_provider = "vsite_domain";
          return TRUE;
        }
      }
    }

    $method = variable_get("purl_method_spaces_{$this->type}", 'path');
    $elements = purl_active()->get($method);
    foreach ($elements as $element) {
      if ($element->provider == "spaces_{$this->type}" && $element->id == $this->id) {
        $this->purl_provider = "spaces_{$this->type}";
        return TRUE;
      }
    }

    // No provider is active, lookup from id so redirection works.
    // Returns FALSE if we have reached here.
    $valid_providers = db_select('purl', 'purl')->fields('purl', array(
        'provider',
      ))->condition('id', $this->id, '=')->execute()->fetchCol();

    if (in_array('vsite_domain', $valid_providers)) {
      // This site has a domain provider.
      $this->purl_provider = 'vsite_domain';
    }
    // Otherwise, sets value from db
    elseif (in_array("spaces_{$this->type}", $valid_providers)) {
      // This site has a default provider.
      $this->purl_provider = "spaces_{$this->type}";
    }

    return FALSE;
  }

  /**
   * Overrides excluded_paths().
   *
   * The parent exlcudes 'features' and 'features/*'
   * We want them back
   */
  protected function excluded_paths() {
    $excluded = parent::excluded_paths();

    unset($excluded[array_search('features', $excluded)]);
    unset($excluded[array_search('features/*', $excluded)]);

    $more = array(
      'user',
      'user/*',
      'users/*',
      'private_site',
      'help/*',
      'site/register',
      'welcome',
      'contact',
      'node',
      'admin',
      'admin/*',
      // 'files/*', Need files menu entry to be loaded in a space so that the purl can be used to serve old file pats. -RB
    );

    $excluded = array_merge($excluded, $more);
    return $excluded;
  }

  /**
   * Checks to see if a path is excluded.
   *
   * @param string $q
   * @return bool
   */
  public static function is_excluded_path($q) {
    $paths = implode("\n", self::excluded_paths());
    return drupal_match_path($q, $paths) ? TRUE : FALSE;
  }

  /**
   * Ensures pURL modifier is present, otherwise redirects via purl_goto()
   */
  protected function activate_purl_modifier() {
    if (!$this->verify_purl()) {
      $this->purge_request_destination();

      // Identical to spaces_purl_provider, but with dynamic provider.
      // @see spaces_purl_provider()
      if ($this->purl_provider) {
        // Sets the provider to use for redirection
        $options = array(
          'query' => drupal_http_build_query(drupal_get_query_parameters()),
          'purl' => array(
            'provider' => $this->purl_provider,
            'id' => $this->id,
          ),
        );
        $path = $_GET['q'];

        //        $aliased = url($path);
        //        if($path != $aliased && isset($_REQUEST['q']) && $path == $_REQUEST['q']){
        //          // If the path we are going to has an alias, and the current page we are on does not use that
        //          // that alias value.  Then lets skip purl_goto since unaliased paths with a path prefix make it think it's
        //          // in a redirect loop.  ex( GET[q] will hold node/123 for both the aliases and non-aliased paths)
        //          $path = $this->get_absolute_url($path);
        //          drupal_goto($path);
        //        }

        purl_goto($path, $options);
      }
    }
  }

  /**
   * Activates user and modify roles based on the space.
   */
  protected function activate_user_roles() {
    global $user;

    $og_roles = og_get_user_roles($this->group_type, $this->id, $user->uid, FALSE);
    $drupal_roles = user_roles(TRUE);

    // Gets drupal core roles with the same name
    $common_roles = array_intersect($drupal_roles, $og_roles);

    foreach ($common_roles as $role) {
      $rid = array_search($role, $drupal_roles);
      $user->roles[$rid] = $role;
    }

    // Clears any staticly cached permissions
    drupal_static('user_access', NULL, TRUE);
  }

  /**
   * Overrides access_space().
   */
  function access_space($account = NULL) {
    return parent::access_space($account) && (!module_exists('vsite_access') || vsite_access_user_access($this->group, $account));
  }
}

