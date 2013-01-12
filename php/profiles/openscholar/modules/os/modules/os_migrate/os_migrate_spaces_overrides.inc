<?php

/**
 * @file
 * Implements TableCopyMigration for each of the biblio tables we're copying.
 */

 abstract class SpacesOverridesTableMigration extends TableCopyMigration {
  public function __construct($dep_groups = array()) {
    $key_schema = array(
        'type' => array(
            'type' => 'varchar',
            'length' => 64,
            'not null' => TRUE,
        ),
        'id' => array(
            'type' => 'varchar',
            'length' => 255,
            'not null' => TRUE,
        ),
        'object_type' => array(
            'type' => 'varchar',
            'length' => 64,
            'not null' => TRUE,
        ),
        'object_id' => array(
            'type' => 'varchar',
            'length' => 255,
            'not null' => TRUE,
        ),
    );

    $this->table = 'spaces_overrides';
    $this->alias = 's';

    parent::__construct($this->table, $key_schema, MigrateGroup::getInstance('spacesoverrides', $dep_groups), NULL, $this->altCountQuery());
  }

  public function query() {
    $query = parent::query();
    $query->condition('object_type', $this->source_object, '=');
    $query->isNotNull('value');
    $query->distinct();

    if ($restrict = OSMigration::restrict_vsites()) {
      $query->condition('id', $restrict, 'IN');
    }

    return $query;
  }

  public function altCountQuery() {

    $query = $this->query()->countQuery();
    $exp = &$query->getExpressions();
    if(count($exp) && isset($exp['expression'])){
      //Replace the default COUNT(*)
      $exp['expression']['expression'] = "COUNT(DISTINCT `type`, `id`, `object_type`, `object_id`)";
    }

    return $query;
  }

   public function prepareRow(&$row) {
     if ($this->source_object != $this->dest_object) {
       $row->object_type = $this->dest_object;
     }
     return parent::prepareRow($row);
   }

  /**
   * @function _update_module
   *
   * Updates a module name where the scholar prefix was shortened to os.  Takes array of names or a single string.
   */
  public function _update_module($module) {
    $module_map = array(
      'vsite_menus' => 'os',
      'scholar_dvn' => 'os_dataverse',
      'scholar_book' => 'os_booklets',
      'scholar_announcements' => 'os_news',
    );

    if (isset($module_map[$module])) {
      return $module_map[$module];
    }

    return preg_replace('/^scholar_/', 'os_', $module);
  }

  /**
   * @function _update_theme_name()
   *
   * Removes prefix before a scholar theme name.
   *
   * @param string $theme
   *   Name of legacy theme
   * @param Bool $remove_deprecated (default: true)
   *   Should themes that are no longer supported be converted to the default or preserved?
   * @return string
   *   D7 name of theme
   */
  public function _update_theme_name($theme, $remove_deprecated = TRUE) {
    //rbates kshepsle and other theme ->flavor vsites will need some extra attention here.
    $theme = preg_replace('/^scholar_/', '', $theme);

    if ($remove_deprecated && in_array($theme, array('burroughs', 'density'))) {
      $theme = 'cleanblue';
    }

    return $theme;
  }

  //return new region name
  public function _update_region($region) {
    $regions = array(
      'header_main' => 'header_second',
      'header_left' => 'header_first',
      'header_right' => 'header_third',
      'navbar' => 'menu_bar',
      'content_top' => 'content_top',
      'content_left' => 'content_first',
      'content_right' => 'content_second',
      'content_bottom' => 'content_bottom',
      'content' => 'content',
      'left' => 'sidebar_first',
      'right' => 'sidebar_second',
      'footer' => 'footer',
    );
    return isset($regions[$region]) ? $regions[$region] : FALSE;
  }

}

