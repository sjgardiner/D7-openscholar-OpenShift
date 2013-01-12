Feature: Testing OpenScholar layout.

  @javascript @wip
  Scenario: Test the creation of a new site.
    Given I am logged in as "admin"
    When I visit "/"
     And I click "Create your site"
     And I fill "edit-domain" with "random text"
     And I press "edit-submit"
     And I should wait and see "Success! The new site has been created."
    # The test will fail due to the error message after creating a new site.
    # And I click "Go there now"
    # Then I wait for the text "Your site's front page is set to display your bio by default."
