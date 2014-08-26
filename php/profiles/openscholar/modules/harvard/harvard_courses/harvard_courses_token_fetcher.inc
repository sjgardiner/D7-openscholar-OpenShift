<?php
/**
 * @file
 *
 * Provides the Class for Feeds Token Fetcher.
 */
/**
 * Base class for the token fetcher.
 */
class HarwardFetcher extends FeedsFetcher {
  var $baseUrl = '';
  var $data = '';
  var $url_param = '';

  public function fetch(FeedsSource $source) {
    $wrapper = entity_metadata_wrapper('node', $source->feed_nid);
    $nids = haravard_courses_sites_nid_by_import_type($wrapper->field_import_type->value());

    if (empty($nids)) {
      drupal_set_message(t('There were no sites for this import.'));
      return new FeedsFetcherResult('<courses></courses>');
    }

    // Define the query parameter and the field that holds the query values.
    if ($wrapper->field_import_type->value() == 'cat_num') {
      $field = 'field_catalog_number';
      $this->url_param = 'q=';
    }
    else {
      $field = 'field_department_id';
      $this->url_param = 'fq_dept_area_category=dept_area_category:';
    }

    // Loop over the nodes and build the query parameter value.
    $param_values = array();
    foreach ($nids as $nid) {
      $import_wrapper = entity_metadata_wrapper('node', $nid);
      foreach ($import_wrapper->{$field}->value() as $import_values) {
        $import_value[] = $import_values;
      }
    }

    // Build the URL for import and verify for response code 200 from the URL.
    $this->baseUrl = $wrapper->field_base_url->value();
    $rows = 10;
    $start = 0;
    $result = drupal_http_request($this->buildUrl($import_value, $rows, $start));

    preg_match_all('|<courses*.numFound="(.*)">(.*)<\/courses>|U', $result->data, $matches, PREG_PATTERN_ORDER);

    if ($result->code != 200) {
      // oops... error!
      drupal_set_message(t('Unable to get feeds from : ' . $this->buildUrl($import_value, $rows, $start)));
      return new FeedsFetcherResult('<courses></courses>');
    }

    $found = $matches[1][0];
    if ($found < $rows) {
      return new FeedsFetcherResult($result->data);
    }
    else {
      // Number of batches we need to iterate.
      $batches = floor(intval($found) / 10) + 1;
      $courses = '<courses>';
      for ($i = 1; $i <= $batches; $i++) {
        $start = $i == 1 ? 0 : $i * $rows;
        $result = drupal_http_request($this->buildUrl($import_value, $rows, $start));
        preg_match_all('|<courses*.numFound="(.*)">(.*)<\/courses>|U', $result->data, $matches, PREG_PATTERN_ORDER);
        if (!isset($matches[2][0])) {
          continue;
        }
        $courses .= $matches[2][0];
      }
      $courses .= '</courses>';
      return new FeedsFetcherResult($courses);
    }
  }

  /**
   * Building the URL for the XML file.
   */
  private function buildUrl($values, $rows, $start) {
    return $this->baseUrl . urlencode('?' . $this->url_param . '"' . implode('" OR "', $values) . '"&rows=' . $rows . '&start=' . $start);
  }
}
