// vix::middleware::HttpPipeline p;

// p.use(vix::middleware::auth::session({
//   .store = std::make_shared<vix::middleware::auth::InMemorySessionStore>(),
//   .secret = "change-me-32bytes-min",
//   .cookie_name = "sid",
//   .secure = true,
//   .same_site = "Lax"
// }));

// p.use(vix::middleware::auth::jwt({
//   .secret = "jwt-secret",
// }));
// auto *sess = ctx.state_ptr<vix::middleware::auth::Session>();
// if (sess)
// {
//   sess->set("user_id", "123");
// }
