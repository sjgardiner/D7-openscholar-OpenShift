<?php

class OgUsersOSMigration extends OSMigration {

  public function __construct() {
    parent::__construct(MigrateGroup::getInstance('users'));

    $this->description = t('Create user relation to group.');
    $this->dependencies[] = 'VsiteNodeOS';
    $this->dependencies[] = 'UsersOS';

    $this->query = parent::d6_db_select('og_uid', 'ogu')
      ->fields('ogu');

    if ($restrict = OSMigration::restrict_vsites()) {
      $this->query->condition('nid', $restrict, 'IN');
    }


    $source_key = array(
      'nid' => array(
        'type' => 'int',
        'not null' => TRUE,
        'description' => 'node ID of group.',
      ),
      'uid' => array(
        'type' => 'int',
        'not null' => TRUE,
        'description' => 'User ID of member.',
      ),
    );

    $this->map = new MigrateSQLMap($this->machineName, $source_key, MigrateDestinationOGMembership::getKeySchema());
    $this->source = new MigrateSourceSQL($this->query, array(), null, array('map_joinable' => FALSE));
    $this->destination = new MigrateDestinationOSUserOGMembership();

    $field_names = array(
      'is_admin',
      'created',
    );
    foreach ($field_names as $field_name) {
      $this->addFieldMapping($field_name, $field_name);
    }

    $this->addFieldMapping('state', 'is_active');

    $this->addFieldMapping('group_type', NULL)->defaultValue('node');
    $this->addFieldMapping('gid', 'nid');

    $this->addFieldMapping('entity_type', NULL)->defaultValue('user');
    $this->addFieldMapping('etid', 'uid');
  }

  public function prepareRow($row) {
    $row->state = !empty($row->state) ? OG_STATE_ACTIVE : OG_STATE_PENDING;
    parent::prepareRow($row);
  }
}

/**
 * Extension of OG Detination to setup OpenScholar Roles
 * Destination class implementing migration into an OG Audience field.
 */
class MigrateDestinationOSUserOGMembership extends MigrateDestinationOGMembership {

  /**
   * Provides some OS specific post-processing to handle roles
   *
   * @param $entity
   *  Object object to build. Prefilled with any fields mapped in the Migration.
   * @param $row
   *  Raw source data object - passed through to prepare/complete handlers.
   * @return array
   *  Array of key fields of the object that was saved if
   *  successful. FALSE on failure.
   */
  public function import(stdClass $entity, stdClass $row) {

    //Standard membership create
    $return = parent::import($entity, $row);

    if ($entity->entity_type != 'user') {
      return $return;
    }

    $cache = &drupal_static(__FUNCTION__, array());
    if (isset($cache["$entity->group_type:$entity->gid"])) {
      $og_roles = $cache["$entity->group_type:$entity->gid"];
    }
    else {
      $og_roles = og_roles($entity->group_type, NULL, $entity->gid);
      $cache["$entity->group_type:$entity->gid"] = $og_roles;
    }

    //If they were previously group admins give them the vsite admin role, otherwise vsite user
    if(!empty($entity->is_admin)){
      $rid = array_search('vsite admin', $og_roles);
    }else{
      $rid = array_search('vsite user', $og_roles);
    }

    if($rid){
      og_role_grant($entity->group_type, $entity->gid, $entity->etid, $rid);
    }

    return $return;
  }

  /**
   * Delete a membership.
   * @param $id
   *  ID to be deleted.
   */
  public function rollback(array $id) {
    migrate_instrument_start('OGMembership Rollback');
    og_ungroup($id['gid'], 'user', $id['uid']);
    migrate_instrument_stop('OGMembership Rollback');
  }
}