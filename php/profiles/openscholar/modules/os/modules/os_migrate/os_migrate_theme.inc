<?php

/**
 * @file
 *
 * Theme map.
 *
 * Old themes were stored in og.og_theme
 * Now they're spaces variables.
 **/


class ThemeOSMigration extends OSMigration {

  public function __construct() {
    /* A handful of custom themes have become flavors.  Set those AFTER flavors are imported */
    $this->dependencies = array('SpacesOverridesVariable');
    $this->description = t('Vsite theme settings');
    $this->destination_schema =       array(
        'type' => array(
          'type' => 'varchar',
          'length' => 64,
        ),
        'id' => array(
          'type' => 'varchar',
          'length' => 255,
        ),
        'object_type' => array(
          'type' => 'varchar',
          'length' => 64,
        ),
        'object_id' => array(
          'type' => 'varchar',
          'length' => 255,
        ),
        'value' => array(
          'type' => 'text',
        ),
      );

    parent::__construct();

    // if desired, supply node with defaults.
    $this->source = $this->source(); //condtion type?
    $this->destination = $this->destination();
    $this->map = $this->map();
    //$this->dependencies = array('UsersOS', 'EmbedInlineOS', 'ImageInlineOS', 'FileInlineOS', 'HTMLInlineOS', 'VsiteNodeOS'); //og when that's done

    //table copy doesn't like defaults.  just set them all in prepare.
    $this->addSimpleMappings(array('id', 'value', 'object_id', 'object_type', 'type'));

    $this->addUnmigratedSources(array('og_theme', 'nid'), t('DNM'));
  }

  /**
   * several helper functions for setting up child nodes.
   * it's not clear to me yet what level of abstraction I'll be working at
   * so for now I'm leaivng it up to the node that implements this class
   * to use the provided options or come up with their own.
   */

  public function query() {
    $query = parent::d6_db_select('og', 'og');
    $query->addField('og', 'og_theme', 'value');
    $query->addField('og', 'nid', 'id');
                    //->fields('og', array('og_theme', 'nid'));
    //filter only vsite objects?

    if ($restrict = OSMigration::restrict_vsites()) {
      $query->condition('nid', $restrict, 'IN');
    }

    return $query;
  }

  public function sourceFields() {
    $source_fields =  array();
    return $source_fields;
  }

  public function source() {
    return new MigrateSourceSQL($this->query(), array('og_theme'), null, array('map_joinable' => FALSE));
  }

  public function destination() {
    //return new MigrateDestinationTable('spaces_overrides'); //
    return new MigrateDestinationTableCopy('spaces_overrides', $this->destination_schema);
  }

  //map provides the schema for the primary key of the source table.  this lets migrate associate a unique source id
  //with a migrated object
  public function map() {
    return new MigrateSQLMap($this->machineName,
        //source primary key
      array(
      	'id' => array(
          'type' => 'int',
          'not null' => TRUE,
          'unsigned' => TRUE,
        ),
        'value' => array(
          'type' => 'varchar',
          'length' => 255,
        ),
      ),
      //MigrateDestinationTable::getKeySchema('spaces_overrides')
      //use the following as dest primary key:
      $this->destination_schema
    );
  }



  /**
   * Implements prepareRow
   *
   * Fetches path from {url_alias}
   */
  function prepareRow($current_row) {
    $prefix = '/^scholar_/';
    if (preg_match($prefix, $current_row->value)) {
      $current_row->value = preg_replace($prefix, '', $current_row->value);
    }

    //several custom themes were made into flavors.
    $this->_custom_theme_to_flavor($current_row);

    //removed: Burroughs, collector, density.

    $themes = list_themes();
    if (!isset($themes[$current_row->value])) {
      parent::saveMessage(t('Ignoring theme, @theme.  It is no longer supported.', array('@theme'=>$current_row->value)), MigrationBase::MESSAGE_NOTICE);
      $current_row->value = 'ballard';
    }
    $current_row->value = serialize($current_row->value);

    $current_row->type = 'og';
    $current_row->object_type = 'variable';
    $current_row->object_id = 'theme_default';
  }


  function _custom_theme_to_flavor(&$row) {
    //get the new theme

    if (parent::is_domain('http://scholar.harvard.edu')) {

      switch ($row->value) {
        case 'rbates':
          $row->value = 'weft';
          $flavor = 'w_african_nostrip';
        break;

        case 'agordon':
          $row->value = 'eloquent';
          $flavor = 'eloquent_sand_modfront';
        break;

        case 'kshepsle':
          $row->value = 'eloquent';
          $flavor = 'eloquent_green_modfront';
        break;

        default:
          return;
      }

      //set the flavor.
      $object_id = 'os_appearance_' . $row->value . '_flavor';
      db_delete('spaces_overrides')
        ->condition('type', 'og')
        ->condition('id', $row->id)
        ->condition('object_type', 'variable')
        ->condition('object_id', $object_id)
        ->execute();

      db_insert('spaces_overrides')->fields(array(
        'type' => 'og',
        'id' => $row->id,
        'object_type' => 'variable',
        'object_id' => $object_id,
        'value' => serialize($flavor),
      ))->execute();
    }

    if (parent::is_domain('http://projects.iq.harvard.edu')) {
      //arts and humanities
      if ($row->id == '9015' && $row->value == 'commodore') {
        $flavor = '3Columns_front';
      }

      // psr theme is an aglet flavor
      if ($row->value == 'psr') {
        $row->value = 'aglet';
        $flavor = 'black_grey';
      }

      //set the flavor.
      if (isset($flavor)) {
        $object_id = 'os_appearance_' . $row->value . '_flavor';
        db_delete('spaces_overrides')
          ->condition('type', 'og')
          ->condition('id', $row->id)
          ->condition('object_type', 'variable')
          ->condition('object_id', $object_id)
          ->execute();

        db_insert('spaces_overrides')->fields(array(
          'type' => 'og',
          'id' => $row->id,
          'object_type' => 'variable',
          'object_id' => $object_id,
          'value' => serialize($flavor),
        ))->execute();
      }

    }
  }
}
