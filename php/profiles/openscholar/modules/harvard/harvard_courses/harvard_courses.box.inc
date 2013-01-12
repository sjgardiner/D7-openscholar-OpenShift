<?php
/**
 * @file
 * harvard_courses.box.inc
 */

/**
 * Implements hook_default_box().
 */
function harvard_courses_default_box() {
  $export = array();

  $box = new stdClass();
  $box->disabled = FALSE; /* Edit this to true to make a default box disabled initially */
  $box->api_version = 1;
  $box->delta = 'harvard_courses_filter';
  $box->plugin_key = 'harvard_courses_filter';
  $box->title = '';
  $box->description = 'Harvard courses filter';
  $box->options = array();
  $export['harvard_courses_filter'] = $box;

  return $export;
}
