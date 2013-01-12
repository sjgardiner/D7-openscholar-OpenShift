<?php

class LinkNodeOSMigration extends AbstractNodeOSMigration {
  public function __construct() {

    $this->addFieldMapping('field_links_link', 'field_link_href_url');
    $this->addFieldMapping('field_links_link:title', 'field_link_href_title');
    $this->addFieldMapping('field_links_link:attributes', 'field_link_href_attributes');

    // Let AbstractNode handle the source, dest, and map.
    parent::__construct('link'); //current types are vsite, article, and page.  lets start with one of those instead?
    $this->description = t('OS Link Nodes');
  }

  public function query($content_type) {
    $query = parent::query($content_type);

    /**
     * Modify the query.  Add joins and fields here.
    */

    // parent::addQueryCCK($query, $content_type);  //shouldn't this be in the parent query?  maybe check if cck is provided first
//     $query->join('content_type_link', 'ct', 'n.vid = ct.vid');
//     $query->fields('ct', array('field_link_href_url', 'field_link_href_title', 'field_link_href_attributes'));

    return $query;
  }


}