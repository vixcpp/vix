-- Create users table
CREATE TABLE IF NOT EXISTS users (
    id BIGINT UNSIGNED NOT NULL AUTO_INCREMENT,
    name  VARCHAR(120) NOT NULL,
    email VARCHAR(190) NOT NULL,
    age   INT NOT NULL,

    PRIMARY KEY (id),
    UNIQUE KEY uq_users_email (email)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
