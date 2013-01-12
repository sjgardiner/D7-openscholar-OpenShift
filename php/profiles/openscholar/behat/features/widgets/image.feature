Feature:
  Testing the image gallery widget.

  @api
  Scenario: Verify that the image gallery widget works fine.
      Given I am logged in as a user with the "administrator" role
       And the widget "Image gallery" is set in the "Publications" page with the following <settings>:
            | Gallery | Kittens | select list |
        And I visit "john/publications"
       Then I should see the images:
            | slideshow1 |
