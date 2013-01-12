Feature:
  Testing the galleries tab.

  @api @wip
  Scenario: Test the Galleries tab
    Given I visit "john"
     When I click "Galleries"
      And I click "Kittens gallery"
     Then I should see the images:
      | slideshow1 |
      | slideshow2 |
      | slideshow3 |

  @api @debug @wip
  Scenario: Test the Galleries tab
    Given I visit "/user"
     Then I should print page
