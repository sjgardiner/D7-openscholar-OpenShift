<?php

/**
 * Theme function for 'os_reader_google_feedapi' link field formatter.
 */
function theme_google_feedapi_formatter_google_feedapi($info){
  _google_feedapi_load_common_feed_js();

  list($id) = entity_extract_ids($info['entity_type'], $info['entity']);

  $feed_div_id = implode("_", array(
    "gf",
    $info['entity_type'],
    $id,
    $info['delta'],
    $info['num_to_show'],
    $info['content_summary'],
    $info['show_content']
  ));

  drupal_add_js(array(
    'googleFeedAPI' => array(
      $feed_div_id => array(
        'url' => $info['element']['url'],
        'num_feeds' => $info['num_to_show'],
        'content_summary' => $info['content_summary'],
        'show_content' => $info['show_content']
      )
    )
  ), 'setting');

  return "<div id='{$feed_div_id}'></div>";
}

function _google_feedapi_load_common_feed_js(){
  static $common_js;
  if (! isset($common_js)) {
    $common_js = true;
    drupal_add_js('https://www.google.com/jsapi?autoload=' . urlencode('{"modules" : [{"name" : "feeds","version" : "1"}]}'), array(
      'type' => 'external',
      'scope' => 'header',
      'group' => JS_LIBRARY,
      'weight' => - 25
    ));

    // Add required JavaScript and Stylesheets for the google feeds
    drupal_add_library('google_feedapi', 'google-feeds');
  }
}