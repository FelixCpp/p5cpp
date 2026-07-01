#include <p5cpp/p5cpp.hpp>
#include <vector>
#include <cmath>

using namespace p5cpp;

constexpr int WIDTH = 1000;
constexpr int HEIGHT = 700;
constexpr float PERCEPTION_R = 80.0f;
constexpr float SEPARATION_R = 20.0f;

constexpr float MAX_SPEED_PREY = 110.0f;
constexpr float MAX_SPEED_PRED = 140.0f;
constexpr float MAX_FORCE = 180.0f;
constexpr float EAT_RADIUS = 16.0f;
constexpr float PREY_ENERGY_MAX = 6.0f;
constexpr float PREY_REGEN = 1.4f;
constexpr float PRED_ENERGY_MAX = 8.0f;
constexpr float PRED_REPRO_E = 6.5f;
constexpr int INIT_PREY = 100;
constexpr int INIT_PRED = 8;

enum class Role {
    Prey,
    Predator,
};

struct Agent
{
    p5cpp::float2 pos;
    p5cpp::float2 vel;
    float energy;
    Role role;
    bool alive = true;

    float maxSpeed() const { return role == Role::Prey ? MAX_SPEED_PREY : MAX_SPEED_PRED; }
};

static p5cpp::float2 steer(p5cpp::float2 current_vel, p5cpp::float2 desired, float maxSpd)
{
    if (desired.length() < 0.0001f) return {0, 0};
    desired = desired.fixedLength(maxSpd);
    p5cpp::float2 f = desired - current_vel;
    return f.limited(MAX_FORCE);
}

struct SimSketch : p5cpp::Sketch
{
    std::vector<Agent> agents;
    std::vector<float> preyHistory, predHistory;
    float histTimer = 0.0f;

    void setup() override
    {
        setWindowSize(WIDTH, HEIGHT);
        setWindowTitle("Predator & Prey — p5cpp");
        frameRate(60);
        p5cpp::randomSeed(1337);
        spawnInitial();
    }

    void spawnInitial()
    {
        agents.clear();
        for (int i = 0; i < INIT_PREY; ++i) spawnAgent(Role::Prey);
        for (int i = 0; i < INIT_PRED; ++i) spawnAgent(Role::Predator);
        preyHistory.clear();
        predHistory.clear();
    }

    void spawnAgent(Role r, p5cpp::float2 pos = {-1, -1})
    {
        Agent a;
        a.role = r;
        a.pos = (pos.x < 0)
                    ? p5cpp::float2 {p5cpp::randomFloat((float)WIDTH), p5cpp::randomFloat((float)HEIGHT)}
                    : pos;
        a.vel = p5cpp::float2::randomUnit() *
                (r == Role::Prey ? MAX_SPEED_PREY : MAX_SPEED_PRED) * 0.5f;
        a.energy = (r == Role::Prey)
                       ? p5cpp::randomFloat(PREY_ENERGY_MAX * 0.3f, PREY_ENERGY_MAX * 0.8f)
                       : p5cpp::randomFloat(PRED_ENERGY_MAX * 0.3f, PRED_ENERGY_MAX * 0.6f);
        a.alive = true;
        agents.push_back(a);
    }

    void draw() override
    {
        const float dt = getDeltaTime();
        update(dt);
        drawScene();
        drawStats();
    }

    void update(float dt)
    {
        for (int i = 0; i < (int)agents.size(); ++i) {
            if (!agents[i].alive || agents[i].role != Role::Predator) continue;
            for (int j = 0; j < (int)agents.size(); ++j) {
                if (!agents[j].alive || agents[j].role != Role::Prey) continue;
                if ((agents[i].pos - agents[j].pos).length() < EAT_RADIUS) {
                    agents[j].alive = false;
                    agents[i].energy += PRED_ENERGY_MAX * 0.4f;
                    if (agents[i].energy > PRED_ENERGY_MAX)
                        agents[i].energy = PRED_ENERGY_MAX;
                }
            }
        }

        std::vector<Agent> newborns;

        for (int i = 0; i < (int)agents.size(); ++i) {
            Agent& a = agents[i];
            if (!a.alive) continue;

            if (a.role == Role::Prey) {
                a.energy += PREY_REGEN * dt;
                if (a.energy > PREY_ENERGY_MAX) a.energy = PREY_ENERGY_MAX;
            }
            a.energy -= dt;
            if (a.energy <= 0.0f) {
                a.alive = false;
                continue;
            }

            if (a.role == Role::Prey && a.energy > PREY_ENERGY_MAX * 0.85f) {
                a.energy *= 0.55f;
                Agent nb;
                nb.role = Role::Prey;
                nb.pos = a.pos + p5cpp::float2::randomUnit() * 10.0f;
                nb.vel = -a.vel * 0.6f;
                nb.energy = PREY_ENERGY_MAX * 0.4f;
                nb.alive = true;
                newborns.push_back(nb);
            }
            if (a.role == Role::Predator && a.energy > PRED_REPRO_E) {
                a.energy *= 0.5f;
                Agent nb;
                nb.role = Role::Predator;
                nb.pos = a.pos + p5cpp::float2::randomUnit() * 12.0f;
                nb.vel = -a.vel * 0.5f;
                nb.energy = PRED_ENERGY_MAX * 0.3f;
                nb.alive = true;
                newborns.push_back(nb);
            }

            p5cpp::float2 steering = (a.role == Role::Prey)
                                         ? preyBehavior(i)
                                         : predatorBehavior(i);
            a.vel = (a.vel + steering * dt).limited(a.maxSpeed());
            a.pos += a.vel * dt;

            if (a.pos.x < 0) a.pos.x += WIDTH;
            if (a.pos.x > WIDTH) a.pos.x -= WIDTH;
            if (a.pos.y < 0) a.pos.y += HEIGHT;
            if (a.pos.y > HEIGHT) a.pos.y -= HEIGHT;
        }

        std::erase_if(agents, [](const Agent& a) {
            return !a.alive;
        });
        for (auto& nb : newborns) agents.push_back(nb);

        if (countRole(Role::Prey) < 5)
            for (int i = 0; i < 5; ++i) spawnAgent(Role::Prey);
        if (countRole(Role::Predator) < 2)
            spawnAgent(Role::Predator);

        histTimer += dt;
        if (histTimer >= 0.5f) {
            histTimer = 0;
            preyHistory.push_back((float)countRole(Role::Prey));
            predHistory.push_back((float)countRole(Role::Predator));
            constexpr int MAX_HIST = 200;
            if ((int)preyHistory.size() > MAX_HIST) preyHistory.erase(preyHistory.begin());
            if ((int)predHistory.size() > MAX_HIST) predHistory.erase(predHistory.begin());
        }
    }

    p5cpp::float2 preyBehavior(int idx)
    {
        const Agent& self = agents[idx];
        p5cpp::float2 flee {0, 0}, sep {0, 0}, coh {0, 0}, ali {0, 0};
        int cohCount = 0, aliCount = 0, fleeCount = 0, sepCount = 0;

        for (int i = 0; i < (int)agents.size(); ++i) {
            if (i == idx || !agents[i].alive) continue;
            const Agent& other = agents[i];
            p5cpp::float2 diff = self.pos - other.pos;
            float d = diff.length();

            if (other.role == Role::Predator && d < PERCEPTION_R * 1.5f) {
                flee += (d > 0.1f) ? diff * (1.0f / (d * d)) : p5cpp::float2::randomUnit();
                fleeCount++;
            }
            if (other.role == Role::Prey && d < PERCEPTION_R) {
                if (d < SEPARATION_R && d > 0.1f) {
                    sep += diff * (1.0f / d);
                    sepCount++;
                }
                coh += other.pos;
                cohCount++;
                ali += other.vel;
                aliCount++;
            }
        }

        p5cpp::float2 total {0, 0};
        if (fleeCount > 0) total += steer(self.vel, flee.normalized() * self.maxSpeed(), self.maxSpeed()) * 3.0f;
        if (sepCount > 0) total += steer(self.vel, sep.normalized() * self.maxSpeed(), self.maxSpeed()) * 1.5f;
        if (cohCount > 0) total += steer(self.vel, (coh * (1.0f / cohCount)) - self.pos, self.maxSpeed()) * 0.8f;
        if (aliCount > 0) total += steer(self.vel, ali * (1.0f / aliCount), self.maxSpeed()) * 1.0f;
        total += p5cpp::float2::randomUnit() * 15.0f;
        return total;
    }

    p5cpp::float2 predatorBehavior(int idx)
    {
        const Agent& self = agents[idx];
        p5cpp::float2 sep {0, 0};
        float closestDist = 1e9f;
        p5cpp::float2 closestPrey {0, 0};
        bool foundPrey = false;
        int sepCount = 0;

        for (int i = 0; i < (int)agents.size(); ++i) {
            if (i == idx || !agents[i].alive) continue;
            const Agent& other = agents[i];
            p5cpp::float2 diff = other.pos - self.pos;
            float d = diff.length();

            if (other.role == Role::Prey && d < PERCEPTION_R * 1.2f) {
                if (d < closestDist) {
                    closestDist = d;
                    closestPrey = other.pos;
                    foundPrey = true;
                }
            }
            if (other.role == Role::Predator && d < SEPARATION_R * 1.5f && d > 0.1f) {
                sep += (self.pos - other.pos) * (1.0f / d);
                sepCount++;
            }
        }

        p5cpp::float2 total {0, 0};
        if (foundPrey)
            total += steer(self.vel, closestPrey - self.pos, self.maxSpeed()) * 2.5f;
        else
            total += p5cpp::float2::randomUnit() * 20.0f;
        if (sepCount > 0)
            total += steer(self.vel, sep.normalized() * self.maxSpeed(), self.maxSpeed()) * 1.2f;
        return total;
    }

    void drawScene()
    {
        background(15, 20, 30);
        noStroke();

        for (const auto& a : agents) {
            if (!a.alive) continue;
            float energyRatio = a.energy /
                                (a.role == Role::Prey ? PREY_ENERGY_MAX : PRED_ENERGY_MAX);
            int alpha = 180 + (int)(75.0f * energyRatio);

            pushMatrix();
            translate(a.pos.x, a.pos.y);
            rotate(std::atan2(a.vel.y, a.vel.x));

            if (a.role == Role::Prey) {
                fill(p5cpp::rgba(60, 200, 80, alpha));
                beginShape();
                vertex(10, 0);
                vertex(-6, 6);
                vertex(-6, -6);
                endShape(p5cpp::ShapeType::polygon);
            } else {
                fill(p5cpp::rgba(220, 50, 50, alpha));
                beginShape();
                vertex(13, 0);
                vertex(-8, 7);
                vertex(-8, -7);
                endShape(p5cpp::ShapeType::polygon);
            }
            popMatrix();
        }
    }

    void drawStats()
    {
        fill(p5cpp::rgba(0, 0, 0, 160));
        noStroke();
        rect(8, 8, 220, 52, 6, 6);

        fill(p5cpp::rgba(80, 220, 100));
        textSize(14);
        textAlign(p5cpp::TextAlign::topLeft);
        text("Prey:   " + std::to_string(countRole(Role::Prey)), 16, 14);

        fill(p5cpp::rgba(220, 70, 70));
        text("Predator: " + std::to_string(countRole(Role::Predator)), 16, 34);

        drawPopulationGraph();
    }

    void drawPopulationGraph()
    {
        if (preyHistory.size() < 2) return;
        constexpr float GX = 10, GY = HEIGHT - 110, GW = 220, GH = 100;

        fill(p5cpp::rgba(0, 0, 0, 140));
        noStroke();
        rect(GX, GY, GW, GH, 4, 4);

        float maxVal = 10;
        for (float v : preyHistory)
            if (v > maxVal) maxVal = v;
        for (float v : predHistory)
            if (v > maxVal) maxVal = v;

        auto drawLine = [&](const std::vector<float>& hist, p5cpp::color_t col) {
            stroke(col);
            strokeWeight(1.5f);
            int n = (int)hist.size();
            for (int i = 1; i < n; ++i) {
                float x0 = GX + (float)(i - 1) / (float)(n - 1) * GW;
                float x1 = GX + (float)(i) / (float)(n - 1) * GW;
                float y0 = GY + GH - hist[i - 1] / maxVal * GH;
                float y1 = GY + GH - hist[i] / maxVal * GH;
                line(x0, y0, x1, y1);
            }
        };

        drawLine(preyHistory, p5cpp::rgba(60, 220, 80));
        drawLine(predHistory, p5cpp::rgba(220, 60, 60));
        noStroke();
    }

    void event(const p5cpp::WindowEvent& e) override
    {
        if (e.type == p5cpp::EventType::keyPress) {
            if (e.keyEvent.key == p5cpp::Key::escape) quit();
            if (e.keyEvent.key == p5cpp::Key::r) spawnInitial();
        }
    }

    int countRole(Role r) const
    {
        int n = 0;
        for (const auto& a : agents)
            if (a.alive && a.role == r) ++n;
        return n;
    }
};

std::unique_ptr<p5cpp::Sketch> p5cpp::createSketch()
{
    return std::make_unique<SimSketch>();
}
