import { OdysseyServer } from "./server.js";
import { Config } from "./config.js";

const server = new OdysseyServer();
server.start(Config.port);

process.on("SIGINT", () => {
  console.log("\n[Server] Shutting down...");
  server.stop();
  process.exit(0);
});

process.on("SIGTERM", () => {
  server.stop();
  process.exit(0);
});
