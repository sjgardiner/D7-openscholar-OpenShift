Feature:
  Testing the managing of OpenScholar

  @api
  Scenario: Check that all of the apps are turned on
    Given I am logged in as a user with the "administrator" role
      And I visit "john"
     When I click "Build"
      And I should see the heading "Apps"
     Then I should see the "spaces" table with the following <contents>:
      | Bio/CV        | Public |
      | Blog          | Public |
      | Booklets      | Public |
      | Classes       | Public |
      | Dataverse     | Public |
      | Events        | Public |
      | Image Gallery | Public |
      | Links         | Public |
      | News          | Public |
      | Basic Pages   | Public |
      | Presentations | Public |
      | Profiles      | Public |
      | Publications  | Public |
      | Reader        | Public |
      | Software      | Public |
