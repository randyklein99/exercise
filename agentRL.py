import numpy as np
import random


# Q-Learning Agent
class QLearningAgent:
    def __init__(self, env, alpha=0.1, gamma=0.99, epsilon=0.1):
        self.env = env
        self.alpha = alpha  # Learning rate
        self.gamma = gamma  # Discount factor
        self.epsilon = epsilon  # Exploration rate
        # Initialize Q-table
        self.q_table = defaultdict(lambda: np.zeros(env.action_space.n))

    def get_action(self, state):
        if random.random() < self.epsilon:
            return self.env.action_space.sample()  # Explore
        else:
            return np.argmax(self.q_table[state])  # Exploit

    def update(self, state, action, reward, next_state, done):
        state = tuple(state)  # Convert state to tuple for hashing
        next_state = tuple(next_state)
        q_value = self.q_table[state][action]
        next_max = np.max(self.q_table[next_state]) if not done else 0
        new_q = q_value + self.alpha * (reward + self.gamma * next_max - q_value)
        self.q_table[state][action] = new_q

    def train(self, num_episodes):
        for episode in range(num_episodes):
            state = self.env.reset()
            total_reward = 0
            done = False
            while not done:
                action = self.get_action(state)
                next_state, reward, done, _ = self.env.step(action)
                self.update(state, action, reward, next_state, done)
                state = next_state
                total_reward += reward
            if episode % 100 == 0:
                print(f"Episode {episode}, Total Reward: {total_reward}")
                self.env.render()


# Train the agent
env = WorkoutRoutineEnv()
agent = QLearningAgent(env, alpha=0.1, gamma=0.99, epsilon=0.1)
agent.train(num_episodes=1000)

# Test the trained agent
state = env.reset()
done = False
while not done:
    action = np.argmax(agent.q_table[tuple(state)])
    state, reward, done, _ = env.step(action)
env.render()
