hunter_upload_password(
    # REPO_OWNER + REPO = https://github.com/ingenue/hunter-cache
    REPO_OWNER "ingenue"
    REPO "hunter-cache-2"

    # USERNAME = https://github.com/ingenue
    USERNAME "ingenue"

    # PASSWORD = GitHub token saved as a secure environment variable
    PASSWORD "$ENV{GITHUB_USER_PASSWORD}"
)
