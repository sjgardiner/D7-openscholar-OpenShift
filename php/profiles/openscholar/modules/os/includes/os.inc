<?php
// $Id$

/**
 * @file os.inc
 * Provides generic OS management and feature functions. This file is included
 * by default.
 */

/**
 * Returns avalible bundles, with option to filter by spaces_features status.
 *
 * @param $privacy
 *  Can be array:
 *    - 0: disabled
 *    - 2: private
 *    - 1: public
 *  TRUE returns all
 *
 * @param $space_type
 * @param $reset
 *   (reset features cache)
 *
 * @return
 *  Array keyed with the bundle ID and the bundle name as value.
 */
function os_get_bundles($include_status = array(1), $space_type = 'og', $reset = FALSE) {
  $return_all = FALSE;
  if ($include_status === TRUE) {
    $return_all = TRUE;
  }
  elseif (!is_array($include_status)) {
    $include_status = array($include_status);
  }

  $bundles = array();
  $app_info = os_app_info();
  $spaces_features = variable_get('spaces_features', array());

  foreach ($app_info as $app_name => $info) {
    if (!isset($info['nodetypes'])) {
      continue;
    }
    $module = $info['module'];
    if (!$return_all && (!isset($spaces_features[$module]) || !in_array($spaces_features[$module], $include_status))) {
      continue;
    }

    $_node_types = _node_types_build();
    foreach ($info['nodetypes'] as $nodetype) {
      if (isset($_node_types->names[$nodetype])) {
        $bundles[$nodetype] = $_node_types->names[$nodetype];
      }
      else {
        $bundles[$nodetype] = $module;
      }
    }
  }

  // Sort the bundles in alphabetical order.
  asort($bundles);

  return $bundles;
}

/**
 * Returns the current active feature based on active contexts.
 *
 * @return object
 * The active feature.
 */
function os_get_active_feature() {
  static $active_feature;

  if (!isset($active_feature)) {
    $features = spaces_features('og');
    $contexts = context_active_contexts();
    if (!empty($contexts) && is_array($contexts)) {
      foreach ($features as $feature) {
        $info = $feature->info['features'];
        if (isset($info['context'])) {
          foreach ($info['context'] as $ctx) {
            if (array_key_exists($ctx, $contexts)) {
              $active_feature = $feature;
              break (2);
            }
          }
        }
      }
    }
  }

  return $active_feature;
}

/**
 * Gets the base context path for a feature.  This is the path that will match
 * to this features context.
 *
 * @param string $app_name
 *   The machine name of the app (a feature module)
 *
 * @return string $path
 */
function os_get_app_path($app_name) {
  $app_info = os_app_info($app_name);

  if (isset($app_info['path'])) {
    $path = $app_info['path'];
    if (strlen($path) > 0) {
      return $path;
    }
  }

  return FALSE;
}

